// camera.cpp
#include "camera.h"
#include <cstring>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <x264.h>
#include "dji_logger.h"
#include <iostream>
T_DjiReturnCode Camera::init(const std::string& devicePath, FrameCallback callback) {
    T_DjiReturnCode returnCode;
    T_DjiOsalHandler* osalHandler = DjiPlatform_GetOsalHandler();

    // 如果已经在运行，先停止
    if (m_isRunning) {
        stop();
    }

    cleanup();

    // 创建信号量和互斥锁
    if (osalHandler->SemaphoreCreate(0, &m_semaphore) != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
        USER_LOG_ERROR("Create semaphore error");
        return DJI_ERROR_SYSTEM_MODULE_CODE_UNKNOWN;
    }

    if (osalHandler->MutexCreate(&m_mutex) != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
        USER_LOG_ERROR("Create mutex error");
        osalHandler->SemaphoreDestroy(m_semaphore);
        return DJI_ERROR_SYSTEM_MODULE_CODE_UNKNOWN;
    }
    if (osalHandler->MutexCreate(&data_mutex) != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
        USER_LOG_ERROR("Create mutex error");
        osalHandler->SemaphoreDestroy(m_semaphore);
        return DJI_ERROR_SYSTEM_MODULE_CODE_UNKNOWN;
    }

    m_devicePath = devicePath;
    m_frameCallback = callback;

    // 初始化V4L2设备
    returnCode = initDevice();
    if (returnCode != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
        USER_LOG_ERROR("Init device failed");
        cleanup();
        return returnCode;
    }

    // 初始化内存映射
    returnCode = initMmap();
    if (returnCode != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
        USER_LOG_ERROR("Init mmap failed");
        cleanup();
        return returnCode;
    }

    // 初始化x264编码器
    returnCode = initX264Encoder();
    if (returnCode != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
        USER_LOG_ERROR("Init x264 encoder failed");
        cleanup();
        return returnCode;
    }

    // 创建捕获线程
    returnCode = osalHandler->TaskCreate("camera_capture", Camera::captureThreadEntry,
                                      2048, this, &m_captureThread);
    if (returnCode != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
        USER_LOG_ERROR("Create capture thread failed");
        cleanup();
        return returnCode;
    }

    m_isRunning = false;
    m_stopFlag = false;

    return DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS;
}

T_DjiReturnCode Camera::initDevice() {
    struct v4l2_capability cap;
    struct v4l2_format fmt;
    
    // 打开设备
    m_fd = open(m_devicePath.c_str(), O_RDWR);
    if (m_fd < 0) {
        USER_LOG_ERROR("Cannot open device");
        return DJI_ERROR_SYSTEM_MODULE_CODE_SYSTEM_ERROR;
    }

    // 查询设备能力
    if (ioctl(m_fd, VIDIOC_QUERYCAP, &cap) < 0) {
        USER_LOG_ERROR("VIDIOC_QUERYCAP failed");
        return DJI_ERROR_SYSTEM_MODULE_CODE_SYSTEM_ERROR;
    }

    // 设置视频格式
    memset(&fmt, 0, sizeof(fmt));
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    fmt.fmt.pix.width = WIDTH;
    fmt.fmt.pix.height = HEIGHT;
    fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;  // 或其他YUV格式
    fmt.fmt.pix.field = V4L2_FIELD_ANY;

    if (ioctl(m_fd, VIDIOC_S_FMT, &fmt) < 0) {
        USER_LOG_ERROR("VIDIOC_S_FMT failed");
        return DJI_ERROR_SYSTEM_MODULE_CODE_SYSTEM_ERROR;
    }

    return DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS;
}

T_DjiReturnCode Camera::initMmap() {
    struct v4l2_requestbuffers req;
    
    // 请求缓冲区
    memset(&req, 0, sizeof(req));
    req.count = BUFFER_COUNT;
    req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    req.memory = V4L2_MEMORY_MMAP;

    if (ioctl(m_fd, VIDIOC_REQBUFS, &req) < 0) {
        USER_LOG_ERROR("VIDIOC_REQBUFS failed");
        return DJI_ERROR_SYSTEM_MODULE_CODE_SYSTEM_ERROR;
    }

    m_buffers = (BufferInfo*)calloc(req.count, sizeof(BufferInfo));
    m_bufferCount = req.count;

    // 设置内存映射
    for (uint32_t i = 0; i < req.count; ++i) {
        struct v4l2_buffer buf;
        memset(&buf, 0, sizeof(buf));
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index = i;

        if (ioctl(m_fd, VIDIOC_QUERYBUF, &buf) < 0) {
            USER_LOG_ERROR("VIDIOC_QUERYBUF failed");
            return DJI_ERROR_SYSTEM_MODULE_CODE_SYSTEM_ERROR;
        }

        m_buffers[i].length = buf.length;
        m_buffers[i].start = mmap(NULL, buf.length,
                                PROT_READ | PROT_WRITE,
                                MAP_SHARED,
                                m_fd, buf.m.offset);

        if (m_buffers[i].start == MAP_FAILED) {
            USER_LOG_ERROR("mmap failed");
            return DJI_ERROR_SYSTEM_MODULE_CODE_SYSTEM_ERROR;
        }
    }

    return DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS;
}

T_DjiReturnCode Camera::initX264Encoder() {
    x264_param_t param;
    x264_picture_t* pic = nullptr;

    // 配置x264参数
    x264_param_default_preset(&param, "ultrafast", "zerolatency");
    param.i_width = WIDTH;
    param.i_height = HEIGHT;
    param.i_fps_num = 50;  // 改为50fps，与相机匹配
    param.i_fps_den = 1;
    param.i_threads = 1;
    param.i_keyint_max = 50; // 调整关键帧间隔
    param.b_repeat_headers = 1;
    
    // 对于黑白视频的优化
    param.rc.i_rc_method = X264_RC_CQP;  // 使用固定QP而不是CRF
    param.rc.i_qp_constant = 23;         // 设置较低的QP值以保持质量
    
    // 色度子采样设置
    param.i_csp = X264_CSP_I420;
    
    // 创建编码器
    m_encoder = x264_encoder_open(&param);
    if (!m_encoder) {
        USER_LOG_ERROR("x264_encoder_open failed");
        return DJI_ERROR_SYSTEM_MODULE_CODE_SYSTEM_ERROR;
    }
    
    // 分配图像结构
    pic = (x264_picture_t*)calloc(1, sizeof(x264_picture_t));
    if (x264_picture_alloc(pic, X264_CSP_I420, WIDTH, HEIGHT) < 0) {
        USER_LOG_ERROR("x264_picture_alloc failed");
        return DJI_ERROR_SYSTEM_MODULE_CODE_SYSTEM_ERROR;
    }

    m_picture = pic;
    return DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS;
}

void* Camera::captureThreadEntry(void* arg) {
    Camera* camera = static_cast<Camera*>(arg);
    camera->captureThread();
    return nullptr;
}

// camera.cpp(续)

void Camera::captureThread() {
    T_DjiOsalHandler* osalHandler = DjiPlatform_GetOsalHandler();
    x264_picture_t pic_out;
    x264_nal_t* nals;
    int i_nals;
    struct v4l2_buffer buf;
    const uint8_t nalHeader[NAL_HEADER_SIZE] = {0x00, 0x00, 0x00, 0x01, 0x09, 0x10};
    uint32_t waitDuration = 1000 / 60; // 30fps
    uint32_t rightNow = 0;
    uint32_t nextFrameTime = 0;

    // 启动视频流
    enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (ioctl(m_fd, VIDIOC_STREAMON, &type) < 0) {
        USER_LOG_ERROR("VIDIOC_STREAMON failed");
        return;
    }

    // 将所有缓冲区加入队列
    for (uint32_t i = 0; i < m_bufferCount; ++i) {
        memset(&buf, 0, sizeof(buf));
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index = i;
        if (ioctl(m_fd, VIDIOC_QBUF, &buf) < 0) {
            USER_LOG_ERROR("Initial VIDIOC_QBUF failed");
            return;
        }
    }

    osalHandler->GetTimeMs(&nextFrameTime);
    nextFrameTime += waitDuration;

    while (!m_stopFlag) {
        osalHandler->TaskSleepMs(1000/60);
        osalHandler->GetTimeMs(&rightNow);
        if (nextFrameTime > rightNow) {
            waitDuration = nextFrameTime - rightNow;
        } else {
            waitDuration = 1000 / 60; // 重置为默认帧间隔
        }

        // 等待新的帧
        memset(&buf, 0, sizeof(buf));
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        
        if (ioctl(m_fd, VIDIOC_DQBUF, &buf) < 0) {
            if (errno == EAGAIN) {
                continue;
            }
            USER_LOG_ERROR("VIDIOC_DQBUF failed");
            break;
        }
        
        
        // 将YUYV转换为I420/YUV420P
        uint8_t* raw_data = (uint8_t*)m_buffers[buf.index].start;
        x264_picture_t* pic = (x264_picture_t*)m_picture;


        
        osalHandler->MutexLock(data_mutex);
        m_currentFrame.resize(buf.length);        // 根据帧大小调整存储大小
        memcpy(m_currentFrame.data(), raw_data, buf.length); // 保存到 m_currentFrame
        osalHandler->MutexUnlock(data_mutex);
        // 找最大最小值
        uint16_t min_value = 0xFFFF;
        uint16_t max_value = 0;

        for (int i = 0; i < HEIGHT; i++) {
            for (int j = 0; j < WIDTH; j++) {
                uint8_t low = raw_data[i * WIDTH * 2 + j * 2];
                uint16_t actual_value = (uint16_t)(low) << 7;
                
                if (actual_value < min_value) min_value = actual_value;
                if (actual_value > max_value) max_value = actual_value;
            }
        }

        // 添加调试信息
        // static int frame_count = 0;
        // if (frame_count++ % 30 == 0) {
        //     USER_LOG_INFO("Value range - Min: 0x%04X (%u), Max: 0x%04X (%u)", 
        //                   min_value, min_value, max_value, max_value);
        // }

        // 计算映射系数
        float scale = (max_value == min_value) ? 0.0f : (255.0f / (max_value - min_value));

        // 执行映射
        for (int i = 0; i < HEIGHT; i++) {
            for (int j = 0; j < WIDTH; j++) {
                uint8_t low = raw_data[i * WIDTH * 2 + j * 2];
                uint16_t actual_value = (uint16_t)(low) << 7;
                
                uint8_t value_8bit = (uint8_t)((actual_value - min_value) * scale);
                
                pic->img.plane[0][i * pic->img.i_stride[0] + j] = value_8bit;
                
                if (i % 2 == 0 && j % 2 == 0) {
                    pic->img.plane[1][i/2 * pic->img.i_stride[1] + j/2] = 128;
                    pic->img.plane[2][i/2 * pic->img.i_stride[2] + j/2] = 128;
                }
            }
        }

        // 编码帧
        int frame_size = x264_encoder_encode((x264_t*)m_encoder, &nals, &i_nals, 
                                           (x264_picture_t*)m_picture, &pic_out);
        
        if (frame_size > 0 && m_frameCallback) {
            // 分配足够的缓冲区来存储编码数据和NAL头
            uint8_t* frameData = (uint8_t*)osalHandler->Malloc(frame_size + NAL_HEADER_SIZE);
            if (frameData) {
                // 复制编码数据
                memcpy(frameData, nals[0].p_payload, frame_size);
                // 在末尾添加NAL头
                memcpy(frameData + frame_size, nalHeader, NAL_HEADER_SIZE);
                
                // 通过回调发送数据
                m_frameCallback(frameData, frame_size + NAL_HEADER_SIZE);
                
                osalHandler->Free(frameData);
            }
        }

        // 将缓冲区重新加入队列
        if (ioctl(m_fd, VIDIOC_QBUF, &buf) < 0) {
            USER_LOG_ERROR("VIDIOC_QBUF failed");
            break;
        }

        osalHandler->GetTimeMs(&nextFrameTime);
        nextFrameTime += waitDuration;
    }

    // 停止视频流
    if (ioctl(m_fd, VIDIOC_STREAMOFF, &type) < 0) {
        USER_LOG_ERROR("VIDIOC_STREAMOFF failed");
    }
}

T_DjiReturnCode Camera::start() {
    T_DjiOsalHandler* osalHandler = DjiPlatform_GetOsalHandler();

    if (osalHandler->MutexLock(m_mutex) != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
        return DJI_ERROR_SYSTEM_MODULE_CODE_UNKNOWN;
    }

    m_isRunning = true;
    m_stopFlag = false;
    osalHandler->SemaphorePost(m_semaphore);

    osalHandler->MutexUnlock(m_mutex);
    return DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS;
}

T_DjiReturnCode Camera::stop() {
    if (!m_isRunning) {
        return DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS;
    }

    m_stopFlag = true;
    m_isRunning = false;
    
    return DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS;
}

void Camera::cleanup() {
    T_DjiOsalHandler* osalHandler = DjiPlatform_GetOsalHandler();

    if (m_captureThread) {
        stop();
        osalHandler->TaskDestroy(m_captureThread);
        m_captureThread = nullptr;
    }

    if (m_semaphore) {
        osalHandler->SemaphoreDestroy(m_semaphore);
        m_semaphore = nullptr;
    }

    if (m_mutex) {
        osalHandler->MutexDestroy(m_mutex);
        m_mutex = nullptr;
    }
    if (data_mutex) {
        osalHandler->MutexDestroy(data_mutex);
        data_mutex = nullptr;
    }

    // 清理V4L2资源
    if (m_buffers) {
        for (uint32_t i = 0; i < m_bufferCount; ++i) {
            if (m_buffers[i].start) {
                munmap(m_buffers[i].start, m_buffers[i].length);
            }
        }
        free(m_buffers);
        m_buffers = nullptr;
    }

    if (m_fd >= 0) {
        close(m_fd);
        m_fd = -1;
    }

    // 清理x264资源
    if (m_picture) {
        x264_picture_clean((x264_picture_t*)m_picture);
        free(m_picture);
        m_picture = nullptr;
    }

    if (m_encoder) {
        x264_encoder_close((x264_t*)m_encoder);
        m_encoder = nullptr;
    }

    m_isRunning = false;
    m_stopFlag = false;
    m_bufferCount = 0;
}
const std::vector<uint8_t>& Camera::getCurrentFrame() const {
    
      
    return m_currentFrame;
    
}