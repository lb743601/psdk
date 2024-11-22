#ifndef CAMERA_STREAM_H
#define CAMERA_STREAM_H

#include <cstdint>
#include <linux/videodev2.h>
#include <x264.h>
#include <string>
#include <functional>
#include <atomic>
#include <chrono>
#include "dji_payload_camera.h"
#include "dji_platform.h"
#include "dji_typedef.h"

// 定义帧数据回调函数类型
using FrameCallback = std::function<void(const uint8_t*, size_t)>;

class CameraStream {
public:
    // 单例模式
    static CameraStream& getInstance() {
        static CameraStream instance;
        return instance;
    }

    // 相机配置
    struct CameraConfig {
        int width;
        int height;
        int fps;
        std::string deviceName;
        uint32_t bufferCount;

        CameraConfig() : 
            width(640),
            height(512),
            fps(30),
            deviceName("/dev/video0"),
            bufferCount(4)
        {}
    };

    // 编码器配置
    struct EncoderConfig {
    std::string preset;
    std::string tune;
    int bitrate;
    int keyframeInterval;
    int threads;
    bool enableBFrame;
    int level;          // 添加level参数
    const char* profile; // 添加profile参数

    EncoderConfig() :
        preset("ultrafast"),
        tune("zerolatency"),
        bitrate(1000),
        keyframeInterval(30),
        threads(1),
        enableBFrame(false),
        level(30),
        profile("baseline")
    {}
};

    // 初始化
    T_DjiReturnCode init(FrameCallback callback,
                        const CameraConfig& camConfig = CameraConfig(),
                        const EncoderConfig& encConfig = EncoderConfig());
    
    // 开始视频流
    T_DjiReturnCode startStream();

    // 停止视频流
    T_DjiReturnCode stopStream();

    // 清理资源
    void cleanup();

    // 状态查询
    bool isStreaming() const { return m_isStreaming; }
    bool isInitialized() const { return m_isInitialized; }
    
    // 获取当前配置
    const CameraConfig& getCameraConfig() const { return m_cameraConfig; }
    const EncoderConfig& getEncoderConfig() const { return m_encoderConfig; }

    // 禁用拷贝和赋值
    CameraStream(const CameraStream&) = delete;
    CameraStream& operator=(const CameraStream&) = delete;

private:
    CameraStream() = default;
    ~CameraStream();

    // 相机相关
    struct Buffer {
        void* start = nullptr;
        size_t length = 0;
    };

    struct CameraContext {
        int fd = -1;
        Buffer* buffers = nullptr;
        uint32_t bufferCount = 0;
        bool isStreaming = false;
    };

    // 编码器相关
    struct EncoderContext {
        x264_t* encoder = nullptr;
        x264_picture_t picIn;
        x264_picture_t picOut;
        uint8_t* yuv420Buffer = nullptr;
        size_t yuv420BufferSize = 0;
    };

    // 私有成员函数
    T_DjiReturnCode initCamera(const CameraConfig& config);
    T_DjiReturnCode initEncoder(const EncoderConfig& config);
    T_DjiReturnCode initBuffers();
    void releaseBuffers();
    
    void convertYUYVtoI420(const uint8_t* yuyv, uint8_t* i420);
    T_DjiReturnCode processFrame();
    static void* streamThreadFunc(void* arg);

    // 性能监控相关
    void updatePerformanceStats(int64_t encodingTime);
    void logPerformanceStats();

    // 私有成员变量
    CameraContext m_camera;
    EncoderContext m_encoder;
    CameraConfig m_cameraConfig;
    EncoderConfig m_encoderConfig;
    FrameCallback m_frameCallback;
    
    std::atomic<bool> m_isStreaming{false};
    std::atomic<bool> m_isInitialized{false};
    T_DjiTaskHandle m_streamThread{nullptr};

    // 性能统计
    struct {
        uint64_t totalFrames = 0;
        uint64_t totalEncodingTime = 0;  // 微秒
        uint64_t maxEncodingTime = 0;    // 微秒
        uint64_t minEncodingTime = UINT64_MAX;  // 微秒
    } m_stats;
};

#endif // CAMERA_STREAM_H