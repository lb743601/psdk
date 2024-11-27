// camera.cpp
#include "camera.h"
#include <opencv2/opencv.hpp>

T_DjiReturnCode Camera::init(const std::string& devicePath, FrameCallback callback) {
    T_DjiReturnCode returnCode;
    T_DjiOsalHandler* osalHandler = DjiPlatform_GetOsalHandler();

    if (m_isRunning) {
        stop();
    }

    cleanup();

    // 创建同步原语
    if (osalHandler->SemaphoreCreate(0, &m_semaphore) != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
        return DJI_ERROR_SYSTEM_MODULE_CODE_UNKNOWN;
    }

    if (osalHandler->MutexCreate(&m_mutex) != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
        osalHandler->SemaphoreDestroy(m_semaphore);
        return DJI_ERROR_SYSTEM_MODULE_CODE_UNKNOWN;
    }

    if (osalHandler->MutexCreate(&data_mutex) != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
        osalHandler->SemaphoreDestroy(m_semaphore);
        osalHandler->MutexDestroy(m_mutex);
        return DJI_ERROR_SYSTEM_MODULE_CODE_UNKNOWN;
    }

    m_frameCallback = callback;

    // 初始化大恒相机
    if (!daheng_camera::initialize()) {
        cleanup();
        return DJI_ERROR_SYSTEM_MODULE_CODE_SYSTEM_ERROR;
    }

    // 初始化x264编码器
    returnCode = initX264Encoder();
    if (returnCode != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
        cleanup();
        return returnCode;
    }

    // 创建捕获线程
    returnCode = osalHandler->TaskCreate("camera_capture", Camera::captureThreadEntry,
                                       2048, this, &m_captureThread);
    if (returnCode != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
        cleanup();
        return returnCode;
    }

    m_isRunning = false;
    m_stopFlag = false;

    return DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS;
}

T_DjiReturnCode Camera::initX264Encoder() {
    x264_param_t param;
    x264_picture_t* pic = nullptr;

    x264_param_default_preset(&param, "ultrafast", "zerolatency");
    param.i_width = WIDTH;
    param.i_height = HEIGHT;
    param.i_fps_num = 30;
    param.i_fps_den = 1;
    param.i_threads = 1;
    param.i_keyint_max = 30;
    param.b_repeat_headers = 1;
    param.rc.i_rc_method = X264_RC_CQP;
    param.rc.i_qp_constant = 23;
    param.i_csp = X264_CSP_I420;

    m_encoder = x264_encoder_open(&param);
    if (!m_encoder) {
        return DJI_ERROR_SYSTEM_MODULE_CODE_SYSTEM_ERROR;
    }

    pic = new x264_picture_t();
    if (x264_picture_alloc(pic, X264_CSP_I420, WIDTH, HEIGHT) < 0) {
        delete pic;
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

void Camera::captureThread() {
    T_DjiOsalHandler* osalHandler = DjiPlatform_GetOsalHandler();
    x264_picture_t pic_out;
    x264_nal_t* nals;
    int i_nals;
    const uint8_t nalHeader[NAL_HEADER_SIZE] = {0x00, 0x00, 0x00, 0x01, 0x09, 0x10};

    // 启动相机
    open();
    stream_on();

    while (!m_stopFlag) {
        osalHandler->TaskSleepMs(1000/30);  // 30fps

        // 获取当前帧
        cv::Mat frame = daheng_camera::getCurrentFrame();
        if (frame.empty()) {
            std::cout<<"empty"<<std::endl;
            continue;
        }

        // 保存当前帧
        osalHandler->MutexLock(data_mutex);
        m_currentFrame.resize(frame.total());
        memcpy(m_currentFrame.data(), frame.data, frame.total());
        osalHandler->MutexUnlock(data_mutex);

        // 直接构建I420数据
        x264_picture_t* pic = static_cast<x264_picture_t*>(m_picture);
        
        // 复制Y平面 (亮度)
        memcpy(pic->img.plane[0], frame.data, WIDTH * HEIGHT);
        
        // 填充U平面和V平面为128 (表示无色彩信息)
        memset(pic->img.plane[1], 128, WIDTH * HEIGHT / 4);
        memset(pic->img.plane[2], 128, WIDTH * HEIGHT / 4);

        // 编码帧
        int frame_size = x264_encoder_encode(static_cast<x264_t*>(m_encoder), 
                                           &nals, &i_nals, pic, &pic_out);

        if (frame_size > 0 && m_frameCallback) {
            uint8_t* frameData = (uint8_t*)osalHandler->Malloc(frame_size + NAL_HEADER_SIZE);
            if (frameData) {
                memcpy(frameData, nals[0].p_payload, frame_size);
                memcpy(frameData + frame_size, nalHeader, NAL_HEADER_SIZE);
                m_frameCallback(frameData, frame_size + NAL_HEADER_SIZE);
                osalHandler->Free(frameData);
            }
        }
    }

    // 停止相机
    stream_off();
    close();
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

    if (m_picture) {
        x264_picture_clean(static_cast<x264_picture_t*>(m_picture));
        delete static_cast<x264_picture_t*>(m_picture);
        m_picture = nullptr;
    }

    if (m_encoder) {
        x264_encoder_close(static_cast<x264_t*>(m_encoder));
        m_encoder = nullptr;
    }

    m_isRunning = false;
    m_stopFlag = false;
}

const std::vector<uint8_t>& Camera::getCurrentFrame() const {
    return m_currentFrame;
}