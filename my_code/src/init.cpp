#include "init.h"
#include "dji_high_speed_data_channel.h"
extern "C" {
#include "utils/util_misc.h"
#include "utils/util_time.h"
#include "utils/util_file.h"
#include "utils/util_buffer.h"
}
#include "camera_stream.h"
#include "videoplayer.h"
#include "camera.h"
#include <sys/stat.h> // For directory creation
static bool shouldExecute = true;

void init()
{
    T_DjiReturnCode returnCode;
    T_DjiOsalHandler osalHandler = {0};
    T_DjiHalUartHandler uartHandler = {0};
    T_DjiHalUsbBulkHandler usbBulkHandler = {0};
    T_DjiLoggerConsole printConsole;
    T_DjiLoggerConsole  localRecordConsole;
    T_DjiFileSystemHandler fileSystemHandler = {0};
    T_DjiSocketHandler socketHandler = {0};
    T_DjiHalNetworkHandler networkHandler = {0};

    networkHandler.NetworkInit = HalNetWork_Init;
    networkHandler.NetworkDeInit = HalNetWork_DeInit;
    networkHandler.NetworkGetDeviceInfo = HalNetWork_GetDeviceInfo;
    //network句柄
    socketHandler.Socket = Osal_Socket;
    socketHandler.Bind = Osal_Bind;
    socketHandler.Close = Osal_Close;
    socketHandler.UdpSendData = Osal_UdpSendData;
    socketHandler.UdpRecvData = Osal_UdpRecvData;
    socketHandler.TcpListen = Osal_TcpListen;
    socketHandler.TcpAccept = Osal_TcpAccept;
    socketHandler.TcpConnect = Osal_TcpConnect;
    socketHandler.TcpSendData = Osal_TcpSendData;
    socketHandler.TcpRecvData = Osal_TcpRecvData;

    osalHandler.TaskCreate = Osal_TaskCreate;
    osalHandler.TaskDestroy = Osal_TaskDestroy;
    osalHandler.TaskSleepMs = Osal_TaskSleepMs;
    osalHandler.MutexCreate = Osal_MutexCreate;
    osalHandler.MutexDestroy = Osal_MutexDestroy;
    osalHandler.MutexLock = Osal_MutexLock;
    osalHandler.MutexUnlock = Osal_MutexUnlock;
    osalHandler.SemaphoreCreate = Osal_SemaphoreCreate;
    osalHandler.SemaphoreDestroy = Osal_SemaphoreDestroy;
    osalHandler.SemaphoreWait = Osal_SemaphoreWait;
    osalHandler.SemaphoreTimedWait = Osal_SemaphoreTimedWait;
    osalHandler.SemaphorePost = Osal_SemaphorePost;
    osalHandler.Malloc = Osal_Malloc;
    osalHandler.Free = Osal_Free;
    osalHandler.GetTimeMs = Osal_GetTimeMs;
    osalHandler.GetTimeUs = Osal_GetTimeUs;
    osalHandler.GetRandomNum = Osal_GetRandomNum;
    

    printConsole.func = DjiUser_PrintConsole;
    printConsole.consoleLevel = DJI_LOGGER_CONSOLE_LOG_LEVEL_INFO;
    printConsole.isSupportColor = true;

    localRecordConsole.consoleLevel = DJI_LOGGER_CONSOLE_LOG_LEVEL_DEBUG;
    localRecordConsole.func = DjiUser_LocalWrite;
    localRecordConsole.isSupportColor = false;

    uartHandler.UartInit = HalUart_Init;
    uartHandler.UartDeInit = HalUart_DeInit;
    uartHandler.UartWriteData = HalUart_WriteData;
    uartHandler.UartReadData = HalUart_ReadData;
    uartHandler.UartGetStatus = HalUart_GetStatus;

    fileSystemHandler.FileOpen = Osal_FileOpen,
    fileSystemHandler.FileClose = Osal_FileClose,
    fileSystemHandler.FileWrite = Osal_FileWrite,
    fileSystemHandler.FileRead = Osal_FileRead,
    fileSystemHandler.FileSync = Osal_FileSync,
    fileSystemHandler.FileSeek = Osal_FileSeek,
    fileSystemHandler.DirOpen = Osal_DirOpen,
    fileSystemHandler.DirClose = Osal_DirClose,
    fileSystemHandler.DirRead = Osal_DirRead,
    fileSystemHandler.Mkdir = Osal_Mkdir,
    fileSystemHandler.Unlink = Osal_Unlink,
    fileSystemHandler.Rename = Osal_Rename,
    fileSystemHandler.Stat = Osal_Stat,

    returnCode = DjiPlatform_RegOsalHandler(&osalHandler);
    if (returnCode != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
        throw std::runtime_error("register osal socket handler error");
    }
    returnCode = DjiPlatform_RegHalUartHandler(&uartHandler);
    if (returnCode != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
        throw std::runtime_error("Register hal uart handler error.");
    }
    returnCode = DjiPlatform_RegHalNetworkHandler(&networkHandler);
    if (returnCode != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
        throw std::runtime_error("Register hal network handler error");
    }
    returnCode = DjiPlatform_RegSocketHandler(&socketHandler);
    if (returnCode != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
        throw std::runtime_error("register osal socket handler error");
    }

    returnCode = DjiPlatform_RegFileSystemHandler(&fileSystemHandler);
    if (returnCode != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
        throw std::runtime_error("Register osal filesystem handler error.");
    }

    if (DjiUser_LocalWriteFsInit(DJI_LOG_PATH) != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
        throw std::runtime_error("File system init error.");
    }

    returnCode = DjiLogger_AddConsole(&printConsole);
    if (returnCode != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
        throw std::runtime_error("Add printf console error.");
    }

    returnCode = DjiLogger_AddConsole(&localRecordConsole);
    if (returnCode != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
        throw std::runtime_error("Add printf console error.");
    }
}
void start_service()
{
    T_DjiUserInfo userInfo;
    T_DjiReturnCode returnCode;
    T_DjiAircraftInfoBaseInfo aircraftInfoBaseInfo;
    T_DjiFirmwareVersion firmwareVersion = {
        .majorVersion = 1,
        .minorVersion = 0,
        .modifyVersion = 0,
        .debugVersion = 0,
    };
    signal(SIGTERM, DjiUser_NormalExitHandler);
    
    strncpy(userInfo.appName, USER_APP_NAME, sizeof(userInfo.appName) - 1);
    memcpy(userInfo.appId, USER_APP_ID, USER_UTIL_MIN(sizeof(userInfo.appId), strlen(USER_APP_ID)));
    memcpy(userInfo.appKey, USER_APP_KEY, USER_UTIL_MIN(sizeof(userInfo.appKey), strlen(USER_APP_KEY)));
    memcpy(userInfo.appLicense, USER_APP_LICENSE,USER_UTIL_MIN(sizeof(userInfo.appLicense), strlen(USER_APP_LICENSE)));
    memcpy(userInfo.baudRate, USER_BAUD_RATE, USER_UTIL_MIN(sizeof(userInfo.baudRate), strlen(USER_BAUD_RATE)));
    strncpy(userInfo.developerAccount, USER_DEVELOPER_ACCOUNT, sizeof(userInfo.developerAccount) - 1);
    
    returnCode = DjiCore_Init(&userInfo);
    if (returnCode != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
        throw std::runtime_error("Core init error.");
    }
 
    returnCode = DjiAircraftInfo_GetBaseInfo(&aircraftInfoBaseInfo);
    if (returnCode != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
        throw std::runtime_error("Get aircraft base info error.");
    }



    returnCode = DjiCore_SetAlias("PSDK_APPALIAS");
    if (returnCode != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
        throw std::runtime_error("Set alias error.");
    }
	
    returnCode = DjiCore_SetFirmwareVersion(firmwareVersion);
    if (returnCode != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
        throw std::runtime_error("Set firmware version error.");
    }

    returnCode = DjiCore_SetSerialNumber("PSDK12345678XX");
    if (returnCode != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
        throw std::runtime_error("Set serial number error");
    }

    returnCode = DjiCore_ApplicationStart();
    if (returnCode != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
        throw std::runtime_error("Start sdk application error.");
    }

    USER_LOG_INFO("Application start.");
    
}

T_DjiReturnCode DjiUser_LocalWrite(const uint8_t *data, uint16_t dataLen)
{
    int32_t realLen;

    if (s_djiLogFile == nullptr) {
        return DJI_ERROR_SYSTEM_MODULE_CODE_UNKNOWN;
    }

    realLen = fwrite(data, 1, dataLen, s_djiLogFile);
    fflush(s_djiLogFile);
    if (realLen == dataLen) {
        return DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS;
    } else {
        return DJI_ERROR_SYSTEM_MODULE_CODE_UNKNOWN;
    }
}


T_DjiReturnCode DjiUser_PrintConsole(const uint8_t *data, uint16_t dataLen)
{
    printf("%s", data);

    return DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS;
}

T_DjiReturnCode DjiUser_LocalWriteFsInit(const char *path)
{
    T_DjiReturnCode djiReturnCode = DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS;
    char filePath[DJI_LOG_PATH_MAX_SIZE];
    char systemCmd[DJI_SYSTEM_CMD_STR_MAX_SIZE];
    char folderName[DJI_LOG_FOLDER_NAME_MAX_SIZE];
    time_t currentTime = time(nullptr);
    struct tm *localTime = localtime(&currentTime);
    uint16_t logFileIndex = 0;
    uint16_t currentLogFileIndex;
    uint8_t ret;

    if (localTime == nullptr) {
        printf("Get local time error.\r\n");
        return DJI_ERROR_SYSTEM_MODULE_CODE_SYSTEM_ERROR;
    }

    if (access(DJI_LOG_FOLDER_NAME, F_OK) != 0) {
        sprintf(folderName, "mkdir %s", DJI_LOG_FOLDER_NAME);
        ret = system(folderName);
        if (ret != 0) {
            printf("Create new log folder error, ret:%d.\r\n", ret);
            return DJI_ERROR_SYSTEM_MODULE_CODE_SYSTEM_ERROR;
        }
    }

    s_djiLogFileCnt = fopen(DJI_LOG_INDEX_FILE_NAME, "rb+");
    if (s_djiLogFileCnt == nullptr) {
        s_djiLogFileCnt = fopen(DJI_LOG_INDEX_FILE_NAME, "wb+");
        if (s_djiLogFileCnt == nullptr) {
            return DJI_ERROR_SYSTEM_MODULE_CODE_SYSTEM_ERROR;
        }
    } else {
        ret = fseek(s_djiLogFileCnt, 0, SEEK_SET);
        if (ret != 0) {
            printf("Seek log count file error, ret: %d, errno: %d.\r\n", ret, errno);
            return DJI_ERROR_SYSTEM_MODULE_CODE_SYSTEM_ERROR;
        }

        ret = fread((uint16_t * ) & logFileIndex, 1, sizeof(uint16_t), s_djiLogFileCnt);
        if (ret != sizeof(uint16_t)) {
            printf("Read log file index error.\r\n");
        }
    }

    currentLogFileIndex = logFileIndex;
    logFileIndex++;

    ret = fseek(s_djiLogFileCnt, 0, SEEK_SET);
    if (ret != 0) {
        printf("Seek log file error, ret: %d, errno: %d.\r\n", ret, errno);
        return DJI_ERROR_SYSTEM_MODULE_CODE_SYSTEM_ERROR;
    }

    ret = fwrite((uint16_t * ) & logFileIndex, 1, sizeof(uint16_t), s_djiLogFileCnt);
    if (ret != sizeof(uint16_t)) {
        printf("Write log file index error.\r\n");
        fclose(s_djiLogFileCnt);
        return DJI_ERROR_SYSTEM_MODULE_CODE_SYSTEM_ERROR;
    }

    fclose(s_djiLogFileCnt);

    sprintf(filePath, "%s_%04d_%04d%02d%02d_%02d-%02d-%02d.log", path, currentLogFileIndex,
            localTime->tm_year + 1900, localTime->tm_mon + 1, localTime->tm_mday,
            localTime->tm_hour, localTime->tm_min, localTime->tm_sec);

    s_djiLogFile = fopen(filePath, "wb+");
    if (s_djiLogFile == nullptr) {
        USER_LOG_ERROR("Open filepath time error.");
        return DJI_ERROR_SYSTEM_MODULE_CODE_SYSTEM_ERROR;
    }

    if (logFileIndex >= DJI_LOG_MAX_COUNT) {
        sprintf(systemCmd, "rm -rf %s_%04d*.log", path, currentLogFileIndex - DJI_LOG_MAX_COUNT);
        ret = system(systemCmd);
        if (ret != 0) {
            printf("Remove file error, ret:%d.\r\n", ret);
            return DJI_ERROR_SYSTEM_MODULE_CODE_SYSTEM_ERROR;
        }
    }

    return djiReturnCode;

}
static void DjiUser_NormalExitHandler(int signalNum)
{
    USER_UTIL_UNUSED(signalNum);
    exit(0);
}

static T_DjiMutexHandle s_commonMutex = {0};
static T_DjiMutexHandle s_zoomMutex = {0};
static T_DjiMutexHandle s_tapZoomMutex = {0};

#include "dji_payload_camera.h"
static T_DjiCameraCommonHandler s_commonHandler;
static T_DjiCameraMediaDownloadPlaybackHandler s_psdkCameraMedia = {0};
static T_DjiReturnCode GetSystemState(T_DjiCameraSystemState *systemState);
static T_DjiReturnCode SetMode(E_DjiCameraMode mode);
static T_DjiReturnCode StartRecordVideo(void);
static T_DjiReturnCode StopRecordVideo(void);
static T_DjiReturnCode StartShootPhoto(void);
static T_DjiReturnCode StopShootPhoto(void);
static T_DjiReturnCode SetShootPhotoMode(E_DjiCameraShootPhotoMode mode);
static T_DjiReturnCode GetShootPhotoMode(E_DjiCameraShootPhotoMode *mode);
static T_DjiReturnCode SetPhotoBurstCount(E_DjiCameraBurstCount burstCount);
static T_DjiReturnCode GetPhotoBurstCount(E_DjiCameraBurstCount *burstCount);
static T_DjiReturnCode SetPhotoTimeIntervalSettings(T_DjiCameraPhotoTimeIntervalSettings settings);
static T_DjiReturnCode GetPhotoTimeIntervalSettings(T_DjiCameraPhotoTimeIntervalSettings *settings);
static T_DjiReturnCode GetSDCardState(T_DjiCameraSDCardState *sdCardState);
static T_DjiReturnCode FormatSDCard(void);
T_DjiReturnCode DjiTest_CameraGetMode(E_DjiCameraMode *mode);
T_DjiReturnCode DjiTest_CameraEmuBaseStartService(void)
{
    T_DjiReturnCode returnCode;
    T_DjiOsalHandler *osalHandler = DjiPlatform_GetOsalHandler();
    

    returnCode = osalHandler->MutexCreate(&s_commonMutex);
    if (returnCode != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
        USER_LOG_ERROR("create mutex used to lock tap zoom arguments error: 0x%08llX", returnCode);
        return returnCode;
    }

    

    returnCode = DjiPayloadCamera_Init();//这个初始化才会在MSDK上显示窗口
    if (returnCode != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
        USER_LOG_ERROR("init payload camera error:0x%08llX", returnCode);
        return returnCode;
    }

    s_commonHandler.GetSystemState = GetSystemState;
    s_commonHandler.SetMode = SetMode;
    s_commonHandler.GetMode = DjiTest_CameraGetMode;
    s_commonHandler.StartRecordVideo = StartRecordVideo;
    s_commonHandler.StopRecordVideo = StopRecordVideo;
    s_commonHandler.StartShootPhoto = StartShootPhoto;
    s_commonHandler.StopShootPhoto = StopShootPhoto;
    s_commonHandler.SetShootPhotoMode = SetShootPhotoMode;
    s_commonHandler.GetShootPhotoMode = GetShootPhotoMode;
    s_commonHandler.SetPhotoBurstCount = SetPhotoBurstCount;
    s_commonHandler.GetPhotoBurstCount = GetPhotoBurstCount;
    s_commonHandler.SetPhotoTimeIntervalSettings = SetPhotoTimeIntervalSettings;
    s_commonHandler.GetPhotoTimeIntervalSettings = GetPhotoTimeIntervalSettings;
    s_commonHandler.GetSDCardState = GetSDCardState;
    s_commonHandler.FormatSDCard = FormatSDCard;




    returnCode = DjiPayloadCamera_RegCommonHandler(&s_commonHandler);
    if (returnCode != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
        USER_LOG_ERROR("camera register common handler error:0x%08llX", returnCode);
    }


    // returnCode = DjiPayloadCamera_SetVideoStreamType(DJI_CAMERA_VIDEO_STREAM_TYPE_H264_DJI_FORMAT);
    // if (returnCode != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
    //     USER_LOG_ERROR("DJI camera set video stream error.");
    //     return DJI_ERROR_SYSTEM_MODULE_CODE_UNKNOWN;
    // }
    return DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS;
}
static T_DjiReturnCode GetSystemState(T_DjiCameraSystemState *systemState)
{
    return DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS;
}
static T_DjiReturnCode SetMode(E_DjiCameraMode mode){
    
    return DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS;
}
static T_DjiReturnCode StartRecordVideo(void){
    USER_LOG_INFO("1111");
    return DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS;
}
static T_DjiReturnCode StopRecordVideo(void){
    return DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS;
}
static T_DjiReturnCode StartShootPhoto(void){
    USER_LOG_INFO("11112323");
    return DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS;
}
static T_DjiReturnCode StopShootPhoto(void){
    return DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS;
}
#include <chrono>
#include <fstream>
#include <iomanip>
#include <sstream>

// 示例代码
static T_DjiReturnCode SetShootPhotoMode(E_DjiCameraShootPhotoMode mode) {
    // Check if the function should execute
    if (!shouldExecute) {
        shouldExecute = true; // Reset the flag for the next call
        USER_LOG_INFO("Skipped this execution due to consecutive trigger.");
        return DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS;
    }

    shouldExecute = false; // Mark that the first execution has occurred

    auto& camera = Camera::getInstance();
    const std::vector<uint8_t>& frame = camera.getCurrentFrame();

    // Check if frame is empty
    if (frame.empty()) {
        USER_LOG_ERROR("Frame is empty, cannot save to file.");
        return DJI_ERROR_SYSTEM_MODULE_CODE_SYSTEM_ERROR;
    }

    // Define the output folder path
    const std::string folderPath = "data";
    
    // Create the folder if it doesn't exist
    struct stat info;
    if (stat(folderPath.c_str(), &info) != 0 || !(info.st_mode & S_IFDIR)) {
        if (mkdir(folderPath.c_str(), 0755) != 0) {
            USER_LOG_ERROR("Failed to create directory: %s", folderPath.c_str());
            return DJI_ERROR_SYSTEM_MODULE_CODE_SYSTEM_ERROR;
        }
    }

    // Get the current time and format it into the file name
    auto now = std::chrono::system_clock::now();
    auto now_time_t = std::chrono::system_clock::to_time_t(now);
    auto now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;

    std::ostringstream filename;
    filename << folderPath << "/frame_"
             << std::put_time(std::localtime(&now_time_t), "%Y%m%d_%H%M%S")
             << "_" << std::setfill('0') << std::setw(3) << now_ms.count()
             << ".raw";

    // Save frame to the file
    std::ofstream outFile(filename.str(), std::ios::binary | std::ios::out);
    if (!outFile.is_open()) {
        USER_LOG_ERROR("Failed to open file for writing: %s", filename.str().c_str());
        return DJI_ERROR_SYSTEM_MODULE_CODE_SYSTEM_ERROR;
    }

    outFile.write(reinterpret_cast<const char*>(frame.data()), frame.size());
    outFile.close();

    USER_LOG_INFO("Frame saved to file: %s", filename.str().c_str());
    return DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS;
}

static T_DjiReturnCode GetShootPhotoMode(E_DjiCameraShootPhotoMode *mode){
    return DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS;
}
static T_DjiReturnCode SetPhotoBurstCount(E_DjiCameraBurstCount burstCount){
    return DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS;
}
static T_DjiReturnCode GetPhotoBurstCount(E_DjiCameraBurstCount *burstCount){
    return DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS;
}
static T_DjiReturnCode SetPhotoTimeIntervalSettings(T_DjiCameraPhotoTimeIntervalSettings settings){
    return DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS;
}
static T_DjiReturnCode GetPhotoTimeIntervalSettings(T_DjiCameraPhotoTimeIntervalSettings *settings){
    return DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS;
}
static T_DjiReturnCode GetSDCardState(T_DjiCameraSDCardState *sdCardState){
    return DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS;
}
static T_DjiReturnCode FormatSDCard(void){
    return DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS;
}
T_DjiReturnCode DjiTest_CameraGetMode(E_DjiCameraMode *mode){
    return DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS;
}

typedef enum {
    TEST_PAYLOAD_CAMERA_MEDIA_PLAY_COMMAND_STOP = 0,
    TEST_PAYLOAD_CAMERA_MEDIA_PLAY_COMMAND_PAUSE = 1,
    TEST_PAYLOAD_CAMERA_MEDIA_PLAY_COMMAND_START = 2,
} E_TestPayloadCameraPlaybackCommand;
typedef struct {
    E_TestPayloadCameraPlaybackCommand command;
    uint32_t timeMs;
    char path[DJI_FILE_PATH_SIZE_MAX];
} T_TestPayloadCameraPlaybackCommand;
static T_UtilBuffer s_mediaPlayCommandBufferHandler = {0};
static uint8_t s_mediaPlayCommandBuffer[sizeof(T_TestPayloadCameraPlaybackCommand) * 32] = {0};
static T_DjiMutexHandle s_mediaPlayCommandBufferMutex = {0};
static T_DjiSemaHandle s_mediaPlayWorkSem = NULL;
static T_DjiTaskHandle s_userSendVideoThread;

static void *UserCameraMedia_SendVideoTask(void *arg);
T_DjiReturnCode media_init(void)
{
    T_DjiOsalHandler *osalHandler = DjiPlatform_GetOsalHandler();
    T_DjiReturnCode returnCode;
    
    auto& camera =Camera::getInstance();
    camera.init("/dev/video0", [](const uint8_t* data, size_t size) {
    const uint32_t MAX_SEND_SIZE = 60000;
    uint32_t lengthOfDataHaveBeenSent = 0;
    T_DjiReturnCode returnCode;
    // for (int i = 0; i < 20; i++) {
    //     printf("%02X ", data[i]); // 按字节写入十六进制数据
    //     }
    //     printf("\n");
    while (size - lengthOfDataHaveBeenSent) {
        uint32_t lengthOfDataToBeSent = USER_UTIL_MIN(MAX_SEND_SIZE, 
                                                     size - lengthOfDataHaveBeenSent);
        
        returnCode = DjiPayloadCamera_SendVideoStream(
            data + lengthOfDataHaveBeenSent,
            lengthOfDataToBeSent
        );
        
        if (returnCode != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
            USER_LOG_ERROR("Send video stream error: 0x%08llX", returnCode);
            break;
        }
        
        lengthOfDataHaveBeenSent += lengthOfDataToBeSent;
    }
});
    auto& player = VideoPlayer::getInstance();
    player.init("PSDK_0006.h264", [](const uint8_t* data, size_t size) {
    // 这里调用DJI的发送函数
        for (int i = 0; i < 20; i++) {
        printf("%02X ", data[i]); // 按字节写入十六进制数据
        }
        printf("\n"); // 每 16 字节换行
            
        
        DjiPayloadCamera_SendVideoStream(data, size);
    });

    // 创建回调函数用于发送视频流
   // 帧回调函数的完整实现
//     auto frameCallback = [](const uint8_t* data, size_t size) {
//     static uint64_t lastFrameTime = 0;
//     static bool firstFrame = true;
//     static FILE* fpHex = nullptr; // 用于存储十六进制数据
//     static FILE* fpBin = nullptr; // 用于存储二进制数据
//     static const uint32_t frameInterval = 1000 / 30; // 30fps
    

//     T_DjiOsalHandler *osalHandler = DjiPlatform_GetOsalHandler();
//     uint32_t currentTime;

//     // 获取当前时间
//     osalHandler->GetTimeMs(&currentTime);

//     // 帧率控制
//     if (lastFrameTime != 0) {
//         uint64_t elapsed = currentTime - lastFrameTime;
//         if (elapsed < frameInterval) {
//             osalHandler->TaskSleepMs(frameInterval - elapsed);
//             osalHandler->GetTimeMs(&currentTime);
//         }
//     }

//     FILE *file = fopen("dataBuffer_hex.log", "a");
// if (file == NULL) {
//     USER_LOG_ERROR("Failed to open file for writing.");
//     return DJI_ERROR_SYSTEM_MODULE_CODE_UNKNOWN;
// }

// // 将 dataBuffer 的内容写入文件，以十六进制格式
// for (int i = 0; i < size; i++) {
//     fprintf(file, "%02X ", data[i]); // 按字节写入十六进制数据
//     if ((i + 1) % 16 == 0) {
//         fprintf(file, "\n"); // 每 16 字节换行
//     }
// }
// fprintf(file, "\n"); // 最后换行，便于区分数据块
// fclose(file);  
    
//     // 发送实际的帧数据
//     T_DjiReturnCode returnCode = DjiPayloadCamera_SendVideoStream(data, size);
//     if (returnCode != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
//         USER_LOG_ERROR("Send frame error: 0x%08llX", returnCode);
        
//     }

//     lastFrameTime = currentTime;

//     // 程序退出时清理资源（注册清理回调）
//     static auto cleanup = []() {
        
//     };
//     static bool registered = ([]() {
//         atexit(cleanup);
//         return true;
//     })();
// };

    //  CameraStream::CameraConfig camConfig;
    // camConfig.width = 640;
    // camConfig.height = 512;  // 修改为480
    // camConfig.fps = 30;
    // camConfig.deviceName = "/dev/video0";
    // camConfig.bufferCount = 4;

    // CameraStream::EncoderConfig encConfig;
    // encConfig.preset = "ultrafast";
    // encConfig.tune = "zerolatency";
    // encConfig.bitrate = 4000;        // 1Mbps对于640x480足够了
    // encConfig.keyframeInterval = 30;  // 1秒一个关键帧
    // encConfig.threads = 1;
    // encConfig.enableBFrame = false;

    // // 初始化CameraStream
    // returnCode = cameraStream.init(frameCallback, camConfig, encConfig);
    // if (returnCode != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
    //     USER_LOG_ERROR("Failed to initialize camera stream");
    //     return returnCode;
    // }
    const T_DjiDataChannelBandwidthProportionOfHighspeedChannel bandwidthProportionOfHighspeedChannel =
        {10, 60, 30};
    T_DjiAircraftInfoBaseInfo aircraftInfoBaseInfo ;
    UtilBuffer_Init(&s_mediaPlayCommandBufferHandler, s_mediaPlayCommandBuffer, sizeof(s_mediaPlayCommandBuffer));
    if (DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS != osalHandler->SemaphoreCreate(0, &s_mediaPlayWorkSem)) {
        USER_LOG_ERROR("SemaphoreCreate(\"%s\") error.", "s_mediaPlayWorkSem");
        return DJI_ERROR_SYSTEM_MODULE_CODE_UNKNOWN;
    }

    if (osalHandler->MutexCreate(&s_mediaPlayCommandBufferMutex) != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
        USER_LOG_ERROR("mutex create error");
        return DJI_ERROR_SYSTEM_MODULE_CODE_UNKNOWN;
    }
    returnCode = DjiHighSpeedDataChannel_SetBandwidthProportion(bandwidthProportionOfHighspeedChannel);
    if (returnCode != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
        USER_LOG_ERROR("Set data channel bandwidth width proportion error.");
        return DJI_ERROR_SYSTEM_MODULE_CODE_UNKNOWN;
    }
    returnCode = osalHandler->TaskCreate("user_camera_media_task", UserCameraMedia_SendVideoTask, 2048,
                                             NULL, &s_userSendVideoThread);
        if (returnCode != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
            USER_LOG_ERROR("user send video task create error.");
            return DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS;
        }
    // returnCode = cameraStream.startStream();
    // if (returnCode != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
    //     USER_LOG_ERROR("Failed to start camera stream");
    //     return returnCode;
    // }
        camera.start();
        //player.play();
        return DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS;
}
static void *UserCameraMedia_SendVideoTask(void *arg)
{
    T_DjiOsalHandler *osalHandler = DjiPlatform_GetOsalHandler();
    T_TestPayloadCameraPlaybackCommand playbackCommand ;
    uint16_t bufferReadSize = 0;
    //auto& cameraStream = CameraStream::getInstance();
    
    while(1) {
        //osalHandler->SemaphoreTimedWait(s_mediaPlayWorkSem, 1000);

        // if (osalHandler->MutexLock(s_mediaPlayCommandBufferMutex) != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
        //     USER_LOG_ERROR("mutex lock error");
        //     continue;
        // }

        // bufferReadSize = UtilBuffer_Get(&s_mediaPlayCommandBufferHandler, 
        //                               (uint8_t *) &playbackCommand,
        //                               sizeof(T_TestPayloadCameraPlaybackCommand));

        // if (osalHandler->MutexUnlock(s_mediaPlayCommandBufferMutex) != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
        //     USER_LOG_ERROR("mutex unlock error");
        //     continue;
        // }

        // if (bufferReadSize != sizeof(T_TestPayloadCameraPlaybackCommand)) {
        //     continue;
        // }

        // 处理控制命令
        
    }

    return NULL;
}

// 清理函数
void media_cleanup()
{
    // 停止并清理CameraStream
    auto& cameraStream = CameraStream::getInstance();
    cameraStream.cleanup();

    // 清理其他资源
    T_DjiOsalHandler *osalHandler = DjiPlatform_GetOsalHandler();
    
    if (s_userSendVideoThread) {
        osalHandler->TaskDestroy(s_userSendVideoThread);
        s_userSendVideoThread = NULL;
    }

    if (s_mediaPlayWorkSem) {
        osalHandler->SemaphoreDestroy(s_mediaPlayWorkSem);
        s_mediaPlayWorkSem = NULL;
    }

    if (s_mediaPlayCommandBufferMutex) {
        osalHandler->MutexDestroy(s_mediaPlayCommandBufferMutex);
        s_mediaPlayCommandBufferMutex = NULL;
    }
}