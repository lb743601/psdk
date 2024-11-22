// videoplayer.h
#pragma once

#include <string>
#include <functional>
#include "dji_platform.h"
#include "dji_typedef.h"

class VideoPlayer {
public:
    static constexpr uint32_t VIDEO_FRAME_MAX_COUNT = 18000;
    static constexpr int NAL_HEADER_SIZE = 6;

    struct VideoFrameInfo {
        float durationS;
        uint32_t positionInFile;
        uint32_t size;
    };

    using FrameCallback = std::function<void(const uint8_t*, size_t)>;

    static VideoPlayer& getInstance() {
        static VideoPlayer instance;
        return instance;
    }

    ~VideoPlayer() {
        cleanup();
    }

    T_DjiReturnCode init(const std::string& videoPath, FrameCallback callback);
    T_DjiReturnCode play();
    T_DjiReturnCode pause();
    T_DjiReturnCode stop();
    void setLoopPlayback(bool loop) { m_loopPlayback = loop; }
    void cleanup();

private:
    VideoPlayer() = default;
    VideoPlayer(const VideoPlayer&) = delete;
    VideoPlayer& operator=(const VideoPlayer&) = delete;

    static void* playbackThreadEntry(void* arg);
    void playbackThread();
    T_DjiReturnCode getFrameRate(float* frameRate);
    T_DjiReturnCode getFrameInfo(VideoFrameInfo* frameInfo, uint32_t maxCount, uint32_t* frameCount);

    std::string m_videoPath;
    FrameCallback m_frameCallback;
    FILE* m_videoFile{nullptr};
    
    T_DjiTaskHandle m_playbackThread{nullptr};
    T_DjiMutexHandle m_mutex{nullptr};
    T_DjiSemaHandle m_semaphore{nullptr};
    
    bool m_isPlaying{false};
    bool m_isPaused{false};
    bool m_stopFlag{false};
    bool m_loopPlayback{false};
    
    float m_frameRate{30.0f};
    uint32_t m_currentFrame{0};
    uint32_t m_frameCount{0};
    VideoFrameInfo* m_frameInfo{nullptr};
};