// videoplayer.cpp
#include "videoplayer.h"
#include "dji_logger.h"
#include <cstring>

T_DjiReturnCode VideoPlayer::init(const std::string& videoPath, FrameCallback callback) {
    T_DjiReturnCode returnCode;
    T_DjiOsalHandler* osalHandler = DjiPlatform_GetOsalHandler();

    // 如果已经在运行，先停止
    if (m_isPlaying) {
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

    // 初始化其他成员
    m_videoPath = videoPath;
    m_frameCallback = callback;

    // 分配帧信息内存
    m_frameInfo = (VideoFrameInfo*)osalHandler->Malloc(VIDEO_FRAME_MAX_COUNT * sizeof(VideoFrameInfo));
    if (m_frameInfo == nullptr) {
        USER_LOG_ERROR("Malloc frame info failed");
        cleanup();
        return DJI_ERROR_SYSTEM_MODULE_CODE_MEMORY_ALLOC_FAILED;
    }

    // 打开视频文件
    m_videoFile = fopen(videoPath.c_str(), "rb");
    if (!m_videoFile) {
        USER_LOG_ERROR("Open video file failed");
        cleanup();
        return DJI_ERROR_SYSTEM_MODULE_CODE_SYSTEM_ERROR;
    }

    // 获取帧率
    returnCode = getFrameRate(&m_frameRate);
    if (returnCode != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
        USER_LOG_ERROR("Get frame rate failed");
        cleanup();
        return returnCode;
    }

    // 获取帧信息
    returnCode = getFrameInfo(m_frameInfo, VIDEO_FRAME_MAX_COUNT, &m_frameCount);
    if (returnCode != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
        USER_LOG_ERROR("Get frame info failed");
        cleanup();
        return returnCode;
    }

    // 创建播放线程
    returnCode = osalHandler->TaskCreate("video_playback", VideoPlayer::playbackThreadEntry,
                                       2048, this, &m_playbackThread);
    if (returnCode != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
        USER_LOG_ERROR("Create playback thread failed");
        cleanup();
        return returnCode;
    }

    m_isPlaying = false;
    m_isPaused = false;
    m_stopFlag = false;
    m_currentFrame = 0;

    return DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS;
}

void* VideoPlayer::playbackThreadEntry(void* arg) {
    VideoPlayer* player = static_cast<VideoPlayer*>(arg);
    player->playbackThread();
    return nullptr;
}

void VideoPlayer::playbackThread() {
    T_DjiOsalHandler* osalHandler = DjiPlatform_GetOsalHandler();
    uint8_t* dataBuffer = nullptr;
    uint32_t waitDuration = 1000 / m_frameRate;
    uint32_t rightNow = 0;
    uint32_t nextFrameTime = 0;

    osalHandler->GetTimeMs(&nextFrameTime);
    nextFrameTime += waitDuration;

    while (!m_stopFlag) {
        osalHandler->GetTimeMs(&rightNow);
        if (nextFrameTime > rightNow) {
            waitDuration = nextFrameTime - rightNow;
        } else {
            waitDuration = 1000 / m_frameRate;
        }

        osalHandler->SemaphoreTimedWait(m_semaphore, waitDuration);

        if (m_stopFlag || m_isPaused) {
            continue;
        }

        // 检查是否到达文件尾
        if (m_currentFrame >= m_frameCount) {
            m_currentFrame = 0;
            if (!m_loopPlayback) {
                stop();
                continue;
            }
        }

        // 准备缓冲区
        uint32_t frameBufSize = m_frameInfo[m_currentFrame].size + NAL_HEADER_SIZE;
        dataBuffer = (uint8_t*)osalHandler->Malloc(frameBufSize);
        if (dataBuffer == nullptr) {
            USER_LOG_ERROR("Malloc buffer failed");
            continue;
        }

        // 读取帧数据
        if (fseek(m_videoFile, m_frameInfo[m_currentFrame].positionInFile, SEEK_SET) != 0) {
            USER_LOG_ERROR("Seek frame failed");
            osalHandler->Free(dataBuffer);
            continue;
        }

        size_t dataLen = fread(dataBuffer, 1, m_frameInfo[m_currentFrame].size, m_videoFile);
        if (dataLen != m_frameInfo[m_currentFrame].size) {
            USER_LOG_ERROR("Read frame data failed");
            osalHandler->Free(dataBuffer);
            continue;
        }

        // 添加NAL头
         const uint8_t nalHeader[NAL_HEADER_SIZE] = {0x00, 0x00, 0x00, 0x01, 0x09, 0x10};
         memcpy(dataBuffer + m_frameInfo[m_currentFrame].size, nalHeader, NAL_HEADER_SIZE);

        // 发送数据
        if (m_frameCallback) {
            m_frameCallback(dataBuffer, frameBufSize);
        }

        osalHandler->Free(dataBuffer);

        osalHandler->GetTimeMs(&nextFrameTime);
        nextFrameTime += waitDuration;
        m_currentFrame++;
    }
}

void VideoPlayer::cleanup() {
    T_DjiOsalHandler* osalHandler = DjiPlatform_GetOsalHandler();
    
    if (m_playbackThread) {
        m_stopFlag = true;
        if (m_semaphore) {
            osalHandler->SemaphorePost(m_semaphore);
        }
        osalHandler->TaskDestroy(m_playbackThread);
        m_playbackThread = nullptr;
    }

    if (m_semaphore) {
        osalHandler->SemaphoreDestroy(m_semaphore);
        m_semaphore = nullptr;
    }

    if (m_mutex) {
        osalHandler->MutexDestroy(m_mutex);
        m_mutex = nullptr;
    }

    if (m_frameInfo) {
        osalHandler->Free(m_frameInfo);
        m_frameInfo = nullptr;
    }

    if (m_videoFile) {
        fclose(m_videoFile);
        m_videoFile = nullptr;
    }

    m_isPlaying = false;
    m_isPaused = false;
    m_stopFlag = false;
    m_currentFrame = 0;
}

T_DjiReturnCode VideoPlayer::play() {
    T_DjiOsalHandler* osalHandler = DjiPlatform_GetOsalHandler();

    if (osalHandler->MutexLock(m_mutex) != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
        return DJI_ERROR_SYSTEM_MODULE_CODE_UNKNOWN;
    }

    m_isPaused = false;
    m_isPlaying = true;
    osalHandler->SemaphorePost(m_semaphore);

    osalHandler->MutexUnlock(m_mutex);
    return DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS;
}

T_DjiReturnCode VideoPlayer::pause() {
    T_DjiOsalHandler* osalHandler = DjiPlatform_GetOsalHandler();

    if (osalHandler->MutexLock(m_mutex) != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
        return DJI_ERROR_SYSTEM_MODULE_CODE_UNKNOWN;
    }

    m_isPaused = true;

    osalHandler->MutexUnlock(m_mutex);
    return DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS;
}

T_DjiReturnCode VideoPlayer::stop() {
    m_stopFlag = true;
    return DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS;
}


T_DjiReturnCode VideoPlayer::getFrameRate(float* frameRate) {
    FILE* fpCommand = nullptr;
    char cmdStr[256] = {0};
    int frameRateMolecule = 0;
    int frameRateDenominator = 0;

    snprintf(cmdStr, sizeof(cmdStr), "ffprobe -show_streams \"%s\" 2>/dev/null | grep r_frame_rate", m_videoPath.c_str());
    fpCommand = popen(cmdStr, "r");
    if (fpCommand == nullptr) {
        USER_LOG_ERROR("Execute show frame rate command failed");
        return DJI_ERROR_SYSTEM_MODULE_CODE_SYSTEM_ERROR;
    }

    int ret = fscanf(fpCommand, "r_frame_rate=%d/%d", &frameRateMolecule, &frameRateDenominator);
    pclose(fpCommand);

    if (ret <= 0) {
        USER_LOG_ERROR("Can not find frame rate");
        return DJI_ERROR_SYSTEM_MODULE_CODE_NOT_FOUND;
    }

    *frameRate = (float)frameRateMolecule / (float)frameRateDenominator;
    return DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS;
}

T_DjiReturnCode VideoPlayer::getFrameInfo(VideoFrameInfo* frameInfo, uint32_t maxCount, uint32_t* frameCount) {
    FILE* fpCommand = nullptr;
    char cmdStr[256] = {0};
    T_DjiOsalHandler* osalHandler = DjiPlatform_GetOsalHandler();
    uint32_t frameNumber = 0;
    *frameCount = 0;

    // 分配临时缓冲区用于存储ffprobe输出
    char* frameInfoString = (char*)osalHandler->Malloc(maxCount * 1024);
    if (frameInfoString == nullptr) {
        USER_LOG_ERROR("Malloc memory for frame info failed");
        return DJI_ERROR_SYSTEM_MODULE_CODE_MEMORY_ALLOC_FAILED;
    }
    memset(frameInfoString, 0, maxCount * 1024);

    // 使用ffprobe获取帧信息
    snprintf(cmdStr, sizeof(cmdStr), "ffprobe -show_packets \"%s\" 2>/dev/null", m_videoPath.c_str());
    fpCommand = popen(cmdStr, "r");
    if (fpCommand == nullptr) {
        USER_LOG_ERROR("Execute show frames command failed");
        osalHandler->Free(frameInfoString);
        return DJI_ERROR_SYSTEM_MODULE_CODE_SYSTEM_ERROR;
    }

    // 读取ffprobe输出
    size_t ret = fread(frameInfoString, 1, maxCount * 1024, fpCommand);
    pclose(fpCommand);
    if (ret <= 0) {
        USER_LOG_ERROR("Read show frames command result error");
        osalHandler->Free(frameInfoString);
        return DJI_ERROR_SYSTEM_MODULE_CODE_SYSTEM_ERROR;
    }
    frameInfoString[ret] = '\0';

    // 解析帧信息
    char* frameLocation = frameInfoString;
    const char* frameKeyChar = "[PACKET]";
    const char* durationTimeKeyChar = "duration_time";
    const char* positionKeyChar = "pos";
    const char* sizeKeyChar = "size";
    char frameParameterFormat[50];

    while (1) {
        // 查找帧标记
        frameLocation = strstr(frameLocation, frameKeyChar);
        if (frameLocation == nullptr) {
            break; // 到达文件尾部
        }

        // 查找帧持续时间
        char* durationTimeLocation = strstr(frameLocation, durationTimeKeyChar);
        if (durationTimeLocation) {
            snprintf(frameParameterFormat, sizeof(frameParameterFormat), "%s=%%f", durationTimeKeyChar);
            sscanf(durationTimeLocation, frameParameterFormat, &frameInfo[frameNumber].durationS);
        }

        // 查找帧位置
        char* positionLocation = strstr(frameLocation, positionKeyChar);
        if (positionLocation) {
            snprintf(frameParameterFormat, sizeof(frameParameterFormat), "%s=%%d", positionKeyChar);
            sscanf(positionLocation, frameParameterFormat, &frameInfo[frameNumber].positionInFile);
        }

        // 查找帧大小
        char* sizeLocation = strstr(frameLocation, sizeKeyChar);
        if (sizeLocation) {
            snprintf(frameParameterFormat, sizeof(frameParameterFormat), "%s=%%d", sizeKeyChar);
            sscanf(sizeLocation, frameParameterFormat, &frameInfo[frameNumber].size);
        }

        frameLocation += strlen(frameKeyChar);
        frameNumber++;
        (*frameCount)++;

        if (frameNumber >= maxCount) {
            USER_LOG_ERROR("Frame buffer is full");
            osalHandler->Free(frameInfoString);
            return DJI_ERROR_SYSTEM_MODULE_CODE_OUT_OF_RANGE;
        }
    }

    osalHandler->Free(frameInfoString);

    if (*frameCount == 0) {
        USER_LOG_ERROR("No frames found");
        return DJI_ERROR_SYSTEM_MODULE_CODE_NOT_FOUND;
    }

    return DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS;
}