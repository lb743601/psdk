#pragma once

#include <string>
#include <functional>
#include <vector>
#include "daheng_camera.h"
#include "dji_platform.h"
#include "dji_typedef.h"
#include <opencv2/opencv.hpp>

class Camera {
public:
    static constexpr uint32_t WIDTH = 1280;
    static constexpr uint32_t HEIGHT = 1024;
    static constexpr uint32_t YUV_SIZE = WIDTH * HEIGHT * 3 / 2;
    static constexpr int NAL_HEADER_SIZE = 6;

    using FrameCallback = std::function<void(const uint8_t*, size_t)>;

    static Camera& getInstance() {
        static Camera instance;
        return instance;
    }

    ~Camera() {
        cleanup();
    }

    T_DjiReturnCode init(const std::string& devicePath, FrameCallback callback);
    T_DjiReturnCode start();
    T_DjiReturnCode stop();
    void cleanup();
    
    const std::vector<uint8_t>& getCurrentFrame() const;
    void setExposureTime(double exposureTime);

private:
    Camera() = default;
    Camera(const Camera&) = delete;
    Camera& operator=(const Camera&) = delete;

    static void* encoderThreadEntry(void* arg);
    void encoderThread();
    T_DjiReturnCode initX264Encoder();

    static daheng_camera m_camera;
    FrameCallback m_frameCallback;
    std::vector<uint8_t> m_currentFrame;
    
    T_DjiTaskHandle m_encoderThread{nullptr};
    T_DjiMutexHandle m_mutex{nullptr};
    T_DjiMutexHandle m_dataMutex{nullptr};
    T_DjiSemaHandle m_semaphore{nullptr};
    
    bool m_isRunning{false};
    bool m_stopFlag{false};

    void* m_encoder{nullptr};
    void* m_picture{nullptr};
};