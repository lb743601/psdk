#include "camera_stream.h"
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <string.h>
#include "dji_logger.h"

CameraStream::~CameraStream() {
    cleanup();
}

T_DjiReturnCode CameraStream::init(FrameCallback callback,
                                  const CameraConfig& camConfig,
                                  const EncoderConfig& encConfig) {
    if (m_isInitialized) {
        USER_LOG_ERROR("CameraStream already initialized");
        return DJI_ERROR_SYSTEM_MODULE_CODE_INVALID_PARAMETER;
    }

    m_frameCallback = callback;
    m_cameraConfig = camConfig;
    m_encoderConfig = encConfig;

    // 初始化相机
    T_DjiReturnCode returnCode = initCamera(camConfig);
    if (returnCode != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
        USER_LOG_ERROR("Failed to initialize camera");
        cleanup();
        return returnCode;
    }

    // 初始化编码器
    returnCode = initEncoder(encConfig);
    if (returnCode != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
        USER_LOG_ERROR("Failed to initialize encoder");
        cleanup();
        return returnCode;
    }

    m_isInitialized = true;
    return DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS;
}

T_DjiReturnCode CameraStream::initCamera(const CameraConfig& config) {
    // 打开摄像头设备
    m_camera.fd = open(config.deviceName.c_str(), O_RDWR);
    if (m_camera.fd < 0) {
        USER_LOG_ERROR("Failed to open camera device: %s", config.deviceName.c_str());
        return DJI_ERROR_SYSTEM_MODULE_CODE_SYSTEM_ERROR;
    }

    // 查询设备能力
    struct v4l2_capability cap;
    if (ioctl(m_camera.fd, VIDIOC_QUERYCAP, &cap) < 0) {
        USER_LOG_ERROR("Failed to query camera capabilities");
        return DJI_ERROR_SYSTEM_MODULE_CODE_SYSTEM_ERROR;
    }

    // 设置视频格式
    struct v4l2_format fmt = {0};
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    fmt.fmt.pix.width = config.width;
    fmt.fmt.pix.height = config.height;
    fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;  // 使用YUYV格式
    fmt.fmt.pix.field = V4L2_FIELD_NONE;

    if (ioctl(m_camera.fd, VIDIOC_S_FMT, &fmt) < 0) {
        USER_LOG_ERROR("Failed to set camera format");
        return DJI_ERROR_SYSTEM_MODULE_CODE_SYSTEM_ERROR;
    }

    // 设置帧率
    struct v4l2_streamparm parm = {0};
    parm.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    parm.parm.capture.timeperframe.numerator = 1;
    parm.parm.capture.timeperframe.denominator = config.fps;

    if (ioctl(m_camera.fd, VIDIOC_S_PARM, &parm) < 0) {
        USER_LOG_ERROR("Failed to set camera frame rate");
        return DJI_ERROR_SYSTEM_MODULE_CODE_SYSTEM_ERROR;
    }

    // 初始化缓冲区
    return initBuffers();
}

T_DjiReturnCode CameraStream::initBuffers() {
    struct v4l2_requestbuffers req = {0};
    req.count = m_cameraConfig.bufferCount;
    req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    req.memory = V4L2_MEMORY_MMAP;

    if (ioctl(m_camera.fd, VIDIOC_REQBUFS, &req) < 0) {
        USER_LOG_ERROR("Failed to request camera buffers");
        return DJI_ERROR_SYSTEM_MODULE_CODE_SYSTEM_ERROR;
    }

    m_camera.buffers = new Buffer[req.count];
    m_camera.bufferCount = req.count;

    // 映射缓冲区
    for (uint32_t i = 0; i < req.count; ++i) {
        struct v4l2_buffer buf = {0};
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index = i;

        if (ioctl(m_camera.fd, VIDIOC_QUERYBUF, &buf) < 0) {
            USER_LOG_ERROR("Failed to query camera buffer");
            return DJI_ERROR_SYSTEM_MODULE_CODE_SYSTEM_ERROR;
        }

        m_camera.buffers[i].length = buf.length;
        m_camera.buffers[i].start = mmap(NULL, buf.length,
                                       PROT_READ | PROT_WRITE,
                                       MAP_SHARED,
                                       m_camera.fd, buf.m.offset);

        if (m_camera.buffers[i].start == MAP_FAILED) {
            USER_LOG_ERROR("Failed to mmap camera buffer");
            return DJI_ERROR_SYSTEM_MODULE_CODE_SYSTEM_ERROR;
        }
    }

    return DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS;
}

T_DjiReturnCode CameraStream::initEncoder(const EncoderConfig& config) {
    // 配置x264参数
    x264_param_t param;
    if (x264_param_default_preset(&param, config.preset.c_str(), config.tune.c_str()) < 0) {
        USER_LOG_ERROR("Failed to set x264 preset and tune");
        return DJI_ERROR_SYSTEM_MODULE_CODE_SYSTEM_ERROR;
    }

    // 基本参数设置
    param.i_width = m_cameraConfig.width;
    param.i_height = m_cameraConfig.height;
    
    // 设置Profile和Level
    x264_param_apply_profile(&param, "baseline");
    param.i_level_idc = 30;  // Level 3.0

    // 码率控制
    param.rc.i_rc_method = X264_RC_ABR;  // 平均码率模式
    param.rc.i_bitrate = config.bitrate;
    param.rc.i_vbv_max_bitrate = config.bitrate * 1.2;
    param.rc.i_vbv_buffer_size = config.bitrate;

    // 设置关键帧间隔
    param.i_keyint_max = config.keyframeInterval;
    param.i_keyint_min = config.keyframeInterval;
    param.i_frame_reference = 1;

    // 编码延迟相关
    param.i_bframe = 0;  // 禁用B帧
    param.i_threads = config.threads;
    param.b_sliced_threads = 0;
    param.i_sync_lookahead = 0;
    param.b_vfr_input = 0;
    param.b_repeat_headers = 1;  // 在每个关键帧前放置SPS/PPS
    param.b_annexb = 1;

    // 创建编码器
    m_encoder.encoder = x264_encoder_open(&param);
    if (!m_encoder.encoder) {
        USER_LOG_ERROR("Failed to create x264 encoder");
        return DJI_ERROR_SYSTEM_MODULE_CODE_SYSTEM_ERROR;
    }

    // 分配图像
    if (x264_picture_alloc(&m_encoder.picIn, X264_CSP_I420, 
                          m_cameraConfig.width, m_cameraConfig.height) < 0) {
        USER_LOG_ERROR("Failed to allocate x264 picture");
        x264_encoder_close(m_encoder.encoder);
        m_encoder.encoder = nullptr;
        return DJI_ERROR_SYSTEM_MODULE_CODE_SYSTEM_ERROR;
    }
    
    // 分配YUV420转换缓冲区
    m_encoder.yuv420BufferSize = m_cameraConfig.width * m_cameraConfig.height * 3 / 2;
    m_encoder.yuv420Buffer = new (std::nothrow) uint8_t[m_encoder.yuv420BufferSize];
    if (!m_encoder.yuv420Buffer) {
        USER_LOG_ERROR("Failed to allocate YUV buffer");
        x264_picture_clean(&m_encoder.picIn);
        x264_encoder_close(m_encoder.encoder);
        m_encoder.encoder = nullptr;
        return DJI_ERROR_SYSTEM_MODULE_CODE_SYSTEM_ERROR;
    }

    return DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS;
}
void CameraStream::convertYUYVtoI420(const uint8_t* yuyv, uint8_t* i420) {
    int width = m_cameraConfig.width;
    int height = m_cameraConfig.height;
    
    uint8_t* y = i420;
    uint8_t* u = y + width * height;
    uint8_t* v = u + width * height / 4;
    
    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j += 2) {
            // Y分量
            *y++ = yuyv[i * width * 2 + j * 2];
            *y++ = yuyv[i * width * 2 + j * 2 + 2];
            
            // U和V分量(每4个像素共用)
            if (i % 2 == 0 && j % 2 == 0) {
                *u++ = yuyv[i * width * 2 + j * 2 + 1];
                *v++ = yuyv[i * width * 2 + j * 2 + 3];
            }
        }
    }
}

T_DjiReturnCode CameraStream::startStream() {
    if (!m_isInitialized) {
        USER_LOG_ERROR("CameraStream not initialized");
        return DJI_ERROR_SYSTEM_MODULE_CODE_INVALID_PARAMETER;
    }

    if (m_isStreaming) {
        return DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS;
    }

    // 将缓冲区加入队列
    for (uint32_t i = 0; i < m_camera.bufferCount; ++i) {
        struct v4l2_buffer buf = {0};
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index = i;

        if (ioctl(m_camera.fd, VIDIOC_QBUF, &buf) < 0) {
            USER_LOG_ERROR("Failed to queue camera buffer");
            return DJI_ERROR_SYSTEM_MODULE_CODE_SYSTEM_ERROR;
        }
    }

    // 开始视频捕获
    enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (ioctl(m_camera.fd, VIDIOC_STREAMON, &type) < 0) {
        USER_LOG_ERROR("Failed to start camera capture");
        return DJI_ERROR_SYSTEM_MODULE_CODE_SYSTEM_ERROR;
    }

    m_camera.isStreaming = true;
    m_isStreaming = true;

    // 创建视频处理线程
    T_DjiOsalHandler* osalHandler = DjiPlatform_GetOsalHandler();
    T_DjiReturnCode returnCode = osalHandler->TaskCreate("camera_stream",
                                                        streamThreadFunc,
                                                        4096,
                                                        this,
                                                        &m_streamThread);
    if (returnCode != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
        USER_LOG_ERROR("Failed to create stream thread");
        stopStream();
        return returnCode;
    }

    return DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS;
}

T_DjiReturnCode CameraStream::stopStream() {
    if (!m_isStreaming) {
        return DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS;
    }

    m_isStreaming = false;

    // 停止采集
    if (m_camera.isStreaming) {
        enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        if (ioctl(m_camera.fd, VIDIOC_STREAMOFF, &type) < 0) {
            USER_LOG_ERROR("Failed to stop camera capture");
            return DJI_ERROR_SYSTEM_MODULE_CODE_SYSTEM_ERROR;
        }
        m_camera.isStreaming = false;
    }

    // 等待线程结束
    if (m_streamThread) {
        T_DjiOsalHandler* osalHandler = DjiPlatform_GetOsalHandler();
        osalHandler->TaskDestroy(m_streamThread);
        m_streamThread = nullptr;
    }

    return DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS;
}

void CameraStream::cleanup() {
    // 停止视频流
    stopStream();

    // 释放编码器资源
    if (m_encoder.encoder) {
        x264_picture_clean(&m_encoder.picIn);
        x264_encoder_close(m_encoder.encoder);
        m_encoder.encoder = nullptr;
    }

    // 释放YUV缓冲区
    if (m_encoder.yuv420Buffer) {
        delete[] m_encoder.yuv420Buffer;
        m_encoder.yuv420Buffer = nullptr;
    }

    // 释放相机缓冲区
    if (m_camera.buffers) {
        for (uint32_t i = 0; i < m_camera.bufferCount; ++i) {
            if (m_camera.buffers[i].start) {
                munmap(m_camera.buffers[i].start, m_camera.buffers[i].length);
            }
        }
        delete[] m_camera.buffers;
        m_camera.buffers = nullptr;
    }

    // 关闭相机设备
    if (m_camera.fd >= 0) {
        close(m_camera.fd);
        m_camera.fd = -1;
    }

    m_isInitialized = false;
}

void* CameraStream::streamThreadFunc(void* arg) {
    T_DjiOsalHandler* osalHandler = DjiPlatform_GetOsalHandler();
    auto* instance = static_cast<CameraStream*>(arg);
    struct v4l2_buffer buf = {0};
    x264_nal_t* nals;
    int i_nals;
    uint64_t frameCount = 0;

    while (instance->m_isStreaming) {
        // 1. 从队列中取出缓冲区1
        osalHandler->TaskSleepMs(1000 /30);
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        
        if (ioctl(instance->m_camera.fd, VIDIOC_DQBUF, &buf) < 0) {
            USER_LOG_ERROR("Failed to dequeue camera buffer");
            continue;
        }

        // 记录编码开始时间
        auto encodeStart = std::chrono::high_resolution_clock::now();

        // 2. 转换格式：YUYV到I420(YUV420P)
        instance->convertYUYVtoI420(
            static_cast<uint8_t*>(instance->m_camera.buffers[buf.index].start),
            instance->m_encoder.yuv420Buffer
        );

        // 3. 准备编码输入
        // Y分量
         instance->m_encoder.picIn.img.plane[0] = instance->m_encoder.yuv420Buffer;
        instance->m_encoder.picIn.img.plane[1] = instance->m_encoder.yuv420Buffer + 
                                                instance->m_cameraConfig.width * instance->m_cameraConfig.height;
        instance->m_encoder.picIn.img.plane[2] = instance->m_encoder.yuv420Buffer + 
                                                instance->m_cameraConfig.width * instance->m_cameraConfig.height * 5 / 4;

        instance->m_encoder.picIn.i_pts = frameCount++;

        // 4. H264编码
        x264_picture_t pic_out;
        int frameSize = x264_encoder_encode(instance->m_encoder.encoder,
                                          &nals, &i_nals,
                                          &instance->m_encoder.picIn,
                                          &pic_out);

        if (frameSize < 0) {
            USER_LOG_ERROR("x264 encode error");
            continue;
        }
        
        // 5. 发送编码后的数据
        if (frameSize > 0 && instance->m_frameCallback) {
            for (int i = 0; i < i_nals; i++) {
                if (nals[i].p_payload && nals[i].i_payload > 0) {
                    instance->m_frameCallback(nals[i].p_payload, nals[i].i_payload);
                }
            }
        }

        // 6. 将缓冲区重新加入队列
        if (ioctl(instance->m_camera.fd, VIDIOC_QBUF, &buf) < 0) {
            USER_LOG_ERROR("Failed to requeue camera buffer");
        }

        // 7. 性能监控和日志
        if (frameCount % 300 == 0) {  // 每300帧输出一次统计信息
            instance->logPerformanceStats();
        }
    }

    return nullptr;
}
void CameraStream::updatePerformanceStats(int64_t encodingTime) {
    m_stats.totalFrames++;
    m_stats.totalEncodingTime += encodingTime;
    m_stats.maxEncodingTime = std::max(m_stats.maxEncodingTime, (uint64_t)encodingTime);
    m_stats.minEncodingTime = std::min(m_stats.minEncodingTime, (uint64_t)encodingTime);
}

void CameraStream::logPerformanceStats() {
    if (m_stats.totalFrames == 0) return;

    double avgTime = m_stats.totalEncodingTime / (double)m_stats.totalFrames;
    USER_LOG_DEBUG("Encoding stats - Avg: %.2f ms, Min: %.2f ms, Max: %.2f ms, Frames: %llu",
                   avgTime / 1000.0,
                   m_stats.minEncodingTime / 1000.0,
                   m_stats.maxEncodingTime / 1000.0,
                   m_stats.totalFrames);
    
    // 同时记录当前帧率
    static auto lastLogTime = std::chrono::high_resolution_clock::now();
    auto currentTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::seconds>(currentTime - lastLogTime);
    
    if (duration.count() > 0) {
        double fps = m_stats.totalFrames / (double)duration.count();
        USER_LOG_DEBUG("Current FPS: %.2f", fps);
        lastLogTime = currentTime;
    }
}