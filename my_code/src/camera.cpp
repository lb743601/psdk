#include "camera.h"
#include <cstring>
#include <x264.h>
#include "dji_logger.h"
daheng_camera Camera::m_camera; 
T_DjiReturnCode Camera::init(const std::string& devicePath, FrameCallback callback) {
    T_DjiReturnCode returnCode;
    T_DjiOsalHandler* osalHandler = DjiPlatform_GetOsalHandler();

    if (m_isRunning) {
        stop();
    }

    cleanup();

    // 创建同步对象
    if (osalHandler->SemaphoreCreate(0, &m_semaphore) != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
        USER_LOG_ERROR("Create semaphore error");
        return DJI_ERROR_SYSTEM_MODULE_CODE_UNKNOWN;
    }

    if (osalHandler->MutexCreate(&m_mutex) != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
        USER_LOG_ERROR("Create mutex error");
        osalHandler->SemaphoreDestroy(m_semaphore);
        return DJI_ERROR_SYSTEM_MODULE_CODE_UNKNOWN;
    }

    if (osalHandler->MutexCreate(&m_dataMutex) != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
        USER_LOG_ERROR("Create data mutex error");
        osalHandler->MutexDestroy(m_mutex);
        osalHandler->SemaphoreDestroy(m_semaphore);
        return DJI_ERROR_SYSTEM_MODULE_CODE_UNKNOWN;
    }

    m_frameCallback = callback;

    // 初始化大恒相机
    if (!m_camera.initialize()) {
        USER_LOG_ERROR("Init Daheng camera failed");
        cleanup();
        return DJI_ERROR_SYSTEM_MODULE_CODE_UNKNOWN;
    }

    // 初始化x264编码器
    returnCode = initX264Encoder();
    if (returnCode != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
        USER_LOG_ERROR("Init x264 encoder failed");
        cleanup();
        return returnCode;
    }

    // 创建编码线程
    returnCode = osalHandler->TaskCreate("camera_encoder", Camera::encoderThreadEntry,
                                       2048, this, &m_encoderThread);
    if (returnCode != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
        USER_LOG_ERROR("Create encoder thread failed");
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
    param.i_fps_num = 50;
    param.i_fps_den = 1;
    param.i_threads = 1;
    param.i_keyint_max = 50;
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

void* Camera::encoderThreadEntry(void* arg) {
    Camera* camera = static_cast<Camera*>(arg);
    camera->encoderThread();
    return nullptr;
}

void Camera::encoderThread() {
    T_DjiOsalHandler* osalHandler = DjiPlatform_GetOsalHandler();
    x264_picture_t pic_out;
    x264_nal_t* nals;
    int i_nals;
    const uint8_t nalHeader[NAL_HEADER_SIZE] = {0x00, 0x00, 0x00, 0x01, 0x09, 0x10};
    uint32_t waitDuration = 1000 / 50;  // 50fps
    uint32_t rightNow = 0;
    uint32_t nextFrameTime = 0;

    osalHandler->GetTimeMs(&nextFrameTime);
    nextFrameTime += waitDuration;

    while (!m_stopFlag) {
        osalHandler->TaskSleepMs(1);
        osalHandler->GetTimeMs(&rightNow);
        
        if (nextFrameTime > rightNow) {
            continue;
        }

        // 获取当前帧
        cv::Mat frame = m_camera.getCurrentFrame();
        
        if (frame.empty()) 
        {
            
            continue;

        }

        // 转换为x264需要的格式
        x264_picture_t* pic = (x264_picture_t*)m_picture;
        cv::Mat yuv;
        cv::Mat bgr;
        cv::cvtColor(frame, bgr, cv::COLOR_GRAY2BGR);
        cv::cvtColor(bgr, yuv, cv::COLOR_BGR2YUV_I420);

        // 复制数据到x264图像结构
        osalHandler->MutexLock(m_dataMutex);
        memcpy(pic->img.plane[0], yuv.data, WIDTH * HEIGHT);
        memcpy(pic->img.plane[1], yuv.data + WIDTH * HEIGHT, WIDTH * HEIGHT / 4);
        memcpy(pic->img.plane[2], yuv.data + WIDTH * HEIGHT * 5 / 4, WIDTH * HEIGHT / 4);

        // 保存当前帧数据
        m_currentFrame.resize(frame.total());
        memcpy(m_currentFrame.data(), frame.data, frame.total());
        osalHandler->MutexUnlock(m_dataMutex);

        // 编码
        int frame_size = x264_encoder_encode((x264_t*)m_encoder, &nals, &i_nals, pic, &pic_out);
        
        if (frame_size > 0 && m_frameCallback) {
            uint8_t* frameData = (uint8_t*)osalHandler->Malloc(frame_size + NAL_HEADER_SIZE);
            if (frameData) {
                memcpy(frameData, nals[0].p_payload, frame_size);
                memcpy(frameData + frame_size, nalHeader, NAL_HEADER_SIZE);
                
                m_frameCallback(frameData, frame_size + NAL_HEADER_SIZE);
                
                osalHandler->Free(frameData);
            }
        }

        osalHandler->GetTimeMs(&nextFrameTime);
        nextFrameTime += waitDuration;
    }
}

T_DjiReturnCode Camera::start() {
    T_DjiOsalHandler* osalHandler = DjiPlatform_GetOsalHandler();

    if (osalHandler->MutexLock(m_mutex) != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
        return DJI_ERROR_SYSTEM_MODULE_CODE_UNKNOWN;
    }

    m_camera.open();
    m_camera.stream_on();

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

    m_camera.stream_off();
    m_camera.close();
    
    return DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS;
}

void Camera::cleanup() {
    T_DjiOsalHandler* osalHandler = DjiPlatform_GetOsalHandler();

    if (m_encoderThread) {
        stop();
        osalHandler->TaskDestroy(m_encoderThread);
        m_encoderThread = nullptr;
    }

    if (m_semaphore) {
        osalHandler->SemaphoreDestroy(m_semaphore);
        m_semaphore = nullptr;
    }

    if (m_mutex) {
        osalHandler->MutexDestroy(m_mutex);
        m_mutex = nullptr;
    }

    if (m_dataMutex) {
        osalHandler->MutexDestroy(m_dataMutex);
        m_dataMutex = nullptr;
    }

    if (m_picture) {
        x264_picture_clean((x264_picture_t*)m_picture);
        delete (x264_picture_t*)m_picture;
        m_picture = nullptr;
    }

    if (m_encoder) {
        x264_encoder_close((x264_t*)m_encoder);
        m_encoder = nullptr;
    }

    m_isRunning = false;
    m_stopFlag = false;
}

const std::vector<uint8_t>& Camera::getCurrentFrame() const {
    return m_currentFrame;
}

void Camera::setExposureTime(double exposureTime) {
    m_camera.set_exp(exposureTime);
}