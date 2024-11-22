// camera.h
#pragma once

#include <string>
#include <functional>
#include "dji_platform.h"
#include "dji_typedef.h"
#include <linux/videodev2.h>

class Camera {
public:
    static constexpr uint32_t WIDTH = 640;
    static constexpr uint32_t HEIGHT = 512;
    static constexpr uint32_t BUFFER_COUNT = 10;  // V4L2缓冲区数量
    
    // YUV格式数据 + NAL头
    static constexpr uint32_t YUV_SIZE = WIDTH * HEIGHT * 3 / 2; // YUV420格式
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

private:
    struct BufferInfo {
        void* start;
        size_t length;
    };

    Camera() = default;
    Camera(const Camera&) = delete;
    Camera& operator=(const Camera&) = delete;

    static void* captureThreadEntry(void* arg);
    void captureThread();
    T_DjiReturnCode initDevice();
    T_DjiReturnCode initMmap();
    T_DjiReturnCode initX264Encoder();

    std::string m_devicePath;
    FrameCallback m_frameCallback;
    int m_fd{-1};  // 设备文件描述符
    
    T_DjiTaskHandle m_captureThread{nullptr};
    T_DjiMutexHandle m_mutex{nullptr};
    T_DjiSemaHandle m_semaphore{nullptr};
    
    bool m_isRunning{false};
    bool m_stopFlag{false};
    
    // V4L2缓冲区
    BufferInfo* m_buffers{nullptr};
    uint32_t m_bufferCount{0};

    // x264编码器相关
    void* m_encoder{nullptr};  // x264_t*
    void* m_picture{nullptr};  // x264_picture_t*
};