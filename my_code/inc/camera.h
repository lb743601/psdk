// camera.h
#pragma once

#include "daheng_camera.h"
#include <string>
#include <functional>
#include "dji_platform.h"
#include "dji_typedef.h"
#include <vector>
#include <x264.h>

class Camera : public daheng_camera {
public:
    static constexpr uint32_t WIDTH = 1280;
    static constexpr uint32_t HEIGHT = 1024;
    static constexpr uint32_t BUFFER_COUNT = 10;
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

private:
    Camera() = default;
    Camera(const Camera&) = delete;
    Camera& operator=(const Camera&) = delete;

    static void* captureThreadEntry(void* arg);
    void captureThread();
    T_DjiReturnCode initX264Encoder();

    FrameCallback m_frameCallback;    
    T_DjiTaskHandle m_captureThread{nullptr};
    T_DjiMutexHandle m_mutex{nullptr};
    T_DjiMutexHandle data_mutex{nullptr};
    T_DjiSemaHandle m_semaphore{nullptr};
    
    bool m_isRunning{false};
    bool m_stopFlag{false};
    
    void* m_encoder{nullptr};  // x264_t*
    void* m_picture{nullptr};  // x264_picture_t*
    
    std::vector<uint8_t> m_currentFrame;
};