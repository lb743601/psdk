#include "psdk_init.h"


//--------will be used---
T_DjiReturnCode SetEnvironment(void)
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
    return DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS;
}

T_DjiReturnCode StartApp(void)
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

    returnCode = DjiCore_SetAlias("厉奔的PSDK");
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
    return DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS;
}

T_DjiReturnCode SetCameraEnvironment(void)
{
    T_DjiReturnCode returnCode;
    T_DjiOsalHandler *osalHandler = DjiPlatform_GetOsalHandler();
    returnCode = osalHandler->MutexCreate(&s_commonMutex);
    if (returnCode != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
        USER_LOG_ERROR("create mutex used to lock tap zoom arguments error: 0x%08llX", returnCode);
        return returnCode;
    }
    s_cameraSDCardState.isInserted = true;
    s_cameraSDCardState.isVerified = true;
    s_cameraSDCardState.totalSpaceInMB = SDCARD_TOTAL_SPACE_IN_MB;
    s_cameraSDCardState.remainSpaceInMB = SDCARD_TOTAL_SPACE_IN_MB;
    s_cameraSDCardState.availableCaptureCount = SDCARD_TOTAL_SPACE_IN_MB / SDCARD_PER_PHOTO_SPACE_IN_MB;
    s_cameraSDCardState.availableRecordingTimeInSeconds =
        SDCARD_TOTAL_SPACE_IN_MB / SDCARD_PER_SECONDS_RECORD_SPACE_IN_MB;
    s_cameraState.cameraMode=DJI_CAMERA_MODE_SHOOT_PHOTO;
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
    return DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS;
}

T_DjiReturnCode Camera_Init(void)
{
    T_DjiOsalHandler *osalHandler = DjiPlatform_GetOsalHandler();
    T_DjiReturnCode returnCode;
    auto& camera =Camera::getInstance();
    camera.init("", [](const uint8_t* data, size_t size)
    {
        const uint32_t MAX_SEND_SIZE = 60000;
        uint32_t lengthOfDataHaveBeenSent = 0;
        T_DjiReturnCode returnCode;
        
        while (size - lengthOfDataHaveBeenSent) 
        {
            uint32_t lengthOfDataToBeSent = USER_UTIL_MIN(MAX_SEND_SIZE,size - lengthOfDataHaveBeenSent);
            returnCode = DjiPayloadCamera_SendVideoStream(data + lengthOfDataHaveBeenSent,lengthOfDataToBeSent);
            if (returnCode != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) 
            {
                USER_LOG_ERROR("Send video stream error: 0x%08llX", returnCode);
                break;
            }
            lengthOfDataHaveBeenSent += lengthOfDataToBeSent;
        }
    });
    const T_DjiDataChannelBandwidthProportionOfHighspeedChannel bandwidthProportionOfHighspeedChannel ={10, 60, 30};
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


    
    s_psdkCameraMedia.GetMediaFileDir = GetMediaFileDir;
    s_psdkCameraMedia.GetMediaFileOriginInfo = DjiTest_CameraMediaGetFileInfo;
    s_psdkCameraMedia.GetMediaFileOriginData = GetMediaFileOriginData;

    s_psdkCameraMedia.CreateMediaFileThumbNail = CreateMediaFileThumbNail;
    s_psdkCameraMedia.GetMediaFileThumbNailInfo = GetMediaFileThumbNailInfo;
    s_psdkCameraMedia.GetMediaFileThumbNailData = GetMediaFileThumbNailData;
    s_psdkCameraMedia.DestroyMediaFileThumbNail = DestroyMediaFileThumbNail;

    s_psdkCameraMedia.CreateMediaFileScreenNail = CreateMediaFileScreenNail;
    s_psdkCameraMedia.GetMediaFileScreenNailInfo = GetMediaFileScreenNailInfo;
    s_psdkCameraMedia.GetMediaFileScreenNailData = GetMediaFileScreenNailData;
    s_psdkCameraMedia.DestroyMediaFileScreenNail = DestroyMediaFileScreenNail;

    s_psdkCameraMedia.DeleteMediaFile = DeleteMediaFile;

    s_psdkCameraMedia.SetMediaPlaybackFile = SetMediaPlaybackFile;

    s_psdkCameraMedia.StartMediaPlayback = StartMediaPlayback;
    s_psdkCameraMedia.StopMediaPlayback = StopMediaPlayback;
    s_psdkCameraMedia.PauseMediaPlayback = PauseMediaPlayback;
    s_psdkCameraMedia.SeekMediaPlayback = SeekMediaPlayback;
    s_psdkCameraMedia.GetMediaPlaybackStatus = GetMediaPlaybackStatus;

    s_psdkCameraMedia.StartDownloadNotification = StartDownloadNotification;
    s_psdkCameraMedia.StopDownloadNotification = StopDownloadNotification;
    returnCode = DjiPayloadCamera_RegMediaDownloadPlaybackHandler(&s_psdkCameraMedia);
        if (returnCode != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
            USER_LOG_ERROR("psdk camera media function init error.");
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

        camera.start();

        return DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS;
}

//-----private------
T_DjiReturnCode DjiUser_PrintConsole(const uint8_t *data, uint16_t dataLen)
{
    printf("%s", data);

    return DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS;
}


void DjiUser_NormalExitHandler(int signalNum)
{
    USER_UTIL_UNUSED(signalNum);
    exit(0);
}
T_DjiReturnCode DjiTest_CameraMediaGetFileInfo(const char *filePath, T_DjiCameraMediaFileInfo *fileInfo)
{
    T_DjiReturnCode returnCode;
    T_DjiMediaFileHandle mediaFileHandle;

    returnCode = DjiMediaFile_CreateHandle(filePath, &mediaFileHandle);
    if (returnCode != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
        USER_LOG_ERROR("Media file create handle error stat:0x%08llX", returnCode);
        return returnCode;
    }

    returnCode = DjiMediaFile_GetMediaFileType(mediaFileHandle, &fileInfo->type);
    if (returnCode != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
        USER_LOG_ERROR("Media file get type error stat:0x%08llX", returnCode);
        goto out;
    }

    returnCode = DjiMediaFile_GetMediaFileAttr(mediaFileHandle, &fileInfo->mediaFileAttr);
    if (returnCode != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
        USER_LOG_ERROR("Media file get attr error stat:0x%08llX", returnCode);
        goto out;
    }

    returnCode = DjiMediaFile_GetFileSizeOrg(mediaFileHandle, &fileInfo->fileSize);
    if (returnCode != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
        USER_LOG_ERROR("Media file get size error stat:0x%08llX", returnCode);
        goto out;
    }

out:
    returnCode = DjiMediaFile_DestroyHandle(mediaFileHandle);
    if (returnCode != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
        USER_LOG_ERROR("Media file destroy handle error stat:0x%08llX", returnCode);
        return returnCode;
    }

    return returnCode;
}

void *UserCameraMedia_SendVideoTask(void *arg)
{
    T_DjiOsalHandler *osalHandler = DjiPlatform_GetOsalHandler();
    T_TestPayloadCameraPlaybackCommand playbackCommand ;
    uint16_t bufferReadSize = 0;
    static uint32_t photoCnt = 0;
    bool isStartIntervalPhotoAction = false;
    s_cameraShootPhotoMode = DJI_CAMERA_SHOOT_PHOTO_MODE_SINGLE;
    while(1) {
  
        if (s_cameraState.shootingState != DJI_CAMERA_SHOOTING_PHOTO_IDLE &&
            photoCnt++ > TAKING_PHOTO_SPENT_TIME_MS_EMU / (1000 / PAYLOAD_CAMERA_EMU_TASK_FREQ)) {
            photoCnt = 0;

            //store the photo after shooting finished
            if (s_cameraShootPhotoMode == DJI_CAMERA_SHOOT_PHOTO_MODE_SINGLE) {
                
                s_cameraSDCardState.remainSpaceInMB =
                    s_cameraSDCardState.remainSpaceInMB - SDCARD_PER_PHOTO_SPACE_IN_MB;
                s_cameraState.isStoring = false;
                s_cameraState.shootingState = DJI_CAMERA_SHOOTING_PHOTO_IDLE;
            } else if (s_cameraShootPhotoMode == DJI_CAMERA_SHOOT_PHOTO_MODE_BURST) {
                s_cameraSDCardState.remainSpaceInMB =
                    s_cameraSDCardState.remainSpaceInMB - SDCARD_PER_PHOTO_SPACE_IN_MB * s_cameraBurstCount;
                s_cameraState.isStoring = false;
                s_cameraState.shootingState = DJI_CAMERA_SHOOTING_PHOTO_IDLE;
            } else if (s_cameraShootPhotoMode == DJI_CAMERA_SHOOT_PHOTO_MODE_INTERVAL) {
                if (isStartIntervalPhotoAction == true) {
                    s_cameraState.isStoring = false;
                    s_cameraState.shootingState = DJI_CAMERA_SHOOTING_PHOTO_IDLE;
                    s_cameraSDCardState.remainSpaceInMB =
                        s_cameraSDCardState.remainSpaceInMB - SDCARD_PER_PHOTO_SPACE_IN_MB;
                }
            }
        
    }
    }

    return NULL;
}








T_DjiReturnCode GetSystemState(T_DjiCameraSystemState *systemState)
{
    T_DjiReturnCode returnCode;
    T_DjiOsalHandler *osalHandler = DjiPlatform_GetOsalHandler();

    returnCode = osalHandler->MutexLock(s_commonMutex);
    if (returnCode != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
        USER_LOG_ERROR("lock mutex error: 0x%08llX.", returnCode);
        return returnCode;
    }

    *systemState = s_cameraState;

    returnCode = osalHandler->MutexUnlock(s_commonMutex);
    if (returnCode != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
        USER_LOG_ERROR("unlock mutex error: 0x%08llX.", returnCode);
        return returnCode;
    }

    return DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS;
}
T_DjiReturnCode SetMode(E_DjiCameraMode mode){
    
    return DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS;
}
T_DjiReturnCode StartRecordVideo(void){
    
    return DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS;
}
T_DjiReturnCode StopRecordVideo(void){
       return DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS;
}

T_DjiReturnCode StartShootPhoto(void) {
    T_DjiReturnCode returnCode;
    T_DjiOsalHandler *osalHandler = DjiPlatform_GetOsalHandler();
    
    if(s_cameraShootPhotoMode == DJI_CAMERA_SHOOT_PHOTO_MODE_SINGLE) {
        s_cameraSDCardState.isVerified = false;
        auto& camera = Camera::getInstance();
        const std::vector<uint8_t>& frame = camera.getCurrentFrame();
        
        returnCode = osalHandler->MutexLock(s_commonMutex);
        if (returnCode != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
            USER_LOG_ERROR("lock mutex error: 0x%08llX.", returnCode);
            return returnCode;
        }

        USER_LOG_INFO("start shoot photo");
        s_cameraState.isStoring = true;

        if (s_cameraShootPhotoMode == DJI_CAMERA_SHOOT_PHOTO_MODE_SINGLE) {
            s_cameraState.shootingState = DJI_CAMERA_SHOOTING_SINGLE_PHOTO;
        } else if (s_cameraShootPhotoMode == DJI_CAMERA_SHOOT_PHOTO_MODE_BURST) {
            s_cameraState.shootingState = DJI_CAMERA_SHOOTING_BURST_PHOTO;
        } else if (s_cameraShootPhotoMode == DJI_CAMERA_SHOOT_PHOTO_MODE_INTERVAL) {
            s_cameraState.shootingState = DJI_CAMERA_SHOOTING_INTERVAL_PHOTO;
            s_cameraState.isShootingIntervalStart = true;
            s_cameraState.currentPhotoShootingIntervalTimeInSeconds = s_cameraPhotoTimeIntervalSettings.timeIntervalSeconds;
        }

        returnCode = osalHandler->MutexUnlock(s_commonMutex);
        if (returnCode != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
            USER_LOG_ERROR("unlock mutex error: 0x%08llX.", returnCode);
            return returnCode;
        }

        // Check if frame is empty
        if (frame.empty()) {
            USER_LOG_ERROR("Frame is empty, cannot save to file.");
            return DJI_ERROR_SYSTEM_MODULE_CODE_SYSTEM_ERROR;
        }

        const size_t EXPECTED_WIDTH = 1280;
        const size_t EXPECTED_HEIGHT = 1024;
        const size_t EXPECTED_SIZE = EXPECTED_WIDTH * EXPECTED_HEIGHT;
        
        // Check size for uint8_t data (each pixel is 1 byte)
        if (frame.size() != EXPECTED_SIZE) {
            USER_LOG_ERROR("Frame size mismatch. Expected: %zu, Got: %zu", 
                          EXPECTED_SIZE, frame.size());
            return DJI_ERROR_SYSTEM_MODULE_CODE_SYSTEM_ERROR;
        }

        // Define the output folder paths
        const std::string rawFolderPath = "data";
        const std::string jpgFolderPath = "jpg";

        // Helper function to create folder if it doesn't exist
        auto createFolderIfNotExists = [](const std::string& folderPath) -> bool {
            struct stat info;
            if (stat(folderPath.c_str(), &info) != 0 || !(info.st_mode & S_IFDIR)) {
                return mkdir(folderPath.c_str(), 0755) == 0;
            }
            return true;
        };

        // Ensure both folders exist
        if (!createFolderIfNotExists(rawFolderPath) || !createFolderIfNotExists(jpgFolderPath)) {
            USER_LOG_ERROR("Failed to create necessary directories.");
            return DJI_ERROR_SYSTEM_MODULE_CODE_SYSTEM_ERROR;
        }

        // Get the current time and format it into the file name
        auto now = std::chrono::system_clock::now();
        auto now_time_t = std::chrono::system_clock::to_time_t(now);
        auto now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;

        std::ostringstream filenameBase;
        filenameBase << std::put_time(std::localtime(&now_time_t), "%Y%m%d_%H%M%S")
                     << "_" << std::setfill('0') << std::setw(3) << now_ms.count();

        // Save RAW frame
        std::ostringstream rawFilePath;
        rawFilePath << rawFolderPath << "/frame_" << filenameBase.str() << ".raw";

        std::ofstream rawFile(rawFilePath.str(), std::ios::binary | std::ios::out);
        if (!rawFile.is_open()) {
            USER_LOG_ERROR("Failed to open RAW file for writing: %s", rawFilePath.str().c_str());
            return DJI_ERROR_SYSTEM_MODULE_CODE_SYSTEM_ERROR;
        }

        rawFile.write(reinterpret_cast<const char*>(frame.data()), frame.size());
        rawFile.close();
        USER_LOG_INFO("RAW frame saved to file: %s", rawFilePath.str().c_str());

        // Save as JPEG
        std::ostringstream jpgFilePath;
        jpgFilePath << jpgFolderPath << "/frame_" << filenameBase.str() << ".jpg";

        FILE* jpgFile = fopen(jpgFilePath.str().c_str(), "wb");
        if (!jpgFile) {
            USER_LOG_ERROR("Failed to open JPG file for writing: %s", jpgFilePath.str().c_str());
            return DJI_ERROR_SYSTEM_MODULE_CODE_SYSTEM_ERROR;
        }

        struct jpeg_compress_struct cinfo;
        struct jpeg_error_mgr jerr;

        cinfo.err = jpeg_std_error(&jerr);
        jpeg_create_compress(&cinfo);
        jpeg_stdio_dest(&cinfo, jpgFile);

        cinfo.image_width = EXPECTED_WIDTH;
        cinfo.image_height = EXPECTED_HEIGHT;
        cinfo.input_components = 1;  // Grayscale
        cinfo.in_color_space = JCS_GRAYSCALE;

        jpeg_set_defaults(&cinfo);
        jpeg_set_quality(&cinfo, 90, TRUE);

        jpeg_start_compress(&cinfo, TRUE);

        // Write scanlines directly from the frame data
        JSAMPROW row_pointer[1];
        const uint8_t* frameData = frame.data();
        while (cinfo.next_scanline < cinfo.image_height) {
            row_pointer[0] = const_cast<uint8_t*>(frameData + cinfo.next_scanline * cinfo.image_width);
            jpeg_write_scanlines(&cinfo, row_pointer, 1);
        }

        jpeg_finish_compress(&cinfo);
        jpeg_destroy_compress(&cinfo);
        fclose(jpgFile);

        USER_LOG_INFO("JPEG frame saved to file: %s", jpgFilePath.str().c_str());

        T_DjiCameraMediaFileInfo mediaFileInfo;
        if (DjiTest_CameraMediaGetFileInfo(jpgFilePath.str().c_str(), &mediaFileInfo) == DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
            std::string fullFilePath = JPG_DIR + jpgFilePath.str();
            std::cout << fullFilePath << std::endl;

            returnCode = DjiPayloadCamera_PushAddedMediaFileInfo(
                fullFilePath.c_str(),
                mediaFileInfo
            );

            if(returnCode != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
                USER_LOG_ERROR("Push media file info failed");
            }
        }
        s_cameraSDCardState.isVerified = true;
    }
    else {
        USER_LOG_INFO("not single mode");
    }
    return DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS;
}

T_DjiReturnCode StopShootPhoto(void){
    USER_LOG_INFO("stop shoot photo");
    return DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS;
}
T_DjiReturnCode SetShootPhotoMode(E_DjiCameraShootPhotoMode mode) {
   T_DjiReturnCode returnCode;
    T_DjiOsalHandler *osalHandler = DjiPlatform_GetOsalHandler();

    returnCode = osalHandler->MutexLock(s_commonMutex);
    if (returnCode != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
        USER_LOG_ERROR("lock mutex error: 0x%08llX.", returnCode);
        return returnCode;
    }

    s_cameraShootPhotoMode = mode;
    USER_LOG_INFO("set shoot photo mode:%d", mode);

    returnCode = osalHandler->MutexUnlock(s_commonMutex);
    if (returnCode != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
        USER_LOG_ERROR("unlock mutex error: 0x%08llX.", returnCode);
        return returnCode;
    }

    return DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS;
}
T_DjiReturnCode GetShootPhotoMode(E_DjiCameraShootPhotoMode *mode){
    T_DjiReturnCode returnCode;
    T_DjiOsalHandler *osalHandler = DjiPlatform_GetOsalHandler();

    returnCode = osalHandler->MutexLock(s_commonMutex);
    if (returnCode != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
        USER_LOG_ERROR("lock mutex error: 0x%08llX.", returnCode);
        return returnCode;
    }

    *mode = s_cameraShootPhotoMode;

    returnCode = osalHandler->MutexUnlock(s_commonMutex);
    if (returnCode != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
        USER_LOG_ERROR("unlock mutex error: 0x%08llX.", returnCode);\
        return returnCode;
    }

    return DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS;
}
T_DjiReturnCode SetPhotoBurstCount(E_DjiCameraBurstCount burstCount){
    T_DjiReturnCode returnCode;
    T_DjiOsalHandler *osalHandler = DjiPlatform_GetOsalHandler();

    returnCode = osalHandler->MutexLock(s_commonMutex);
    if (returnCode != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
        USER_LOG_ERROR("lock mutex error: 0x%08llX.", returnCode);
        return returnCode;
    }

    s_cameraBurstCount = burstCount;
    USER_LOG_INFO("set photo burst count:%d", burstCount);

    returnCode = osalHandler->MutexUnlock(s_commonMutex);
    if (returnCode != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
        USER_LOG_ERROR("unlock mutex error: 0x%08llX.", returnCode);
        return returnCode;
    }

    return DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS;
}
T_DjiReturnCode GetPhotoBurstCount(E_DjiCameraBurstCount *burstCount){
    T_DjiReturnCode returnCode;
    T_DjiOsalHandler *osalHandler = DjiPlatform_GetOsalHandler();

    returnCode = osalHandler->MutexLock(s_commonMutex);
    if (returnCode != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
        USER_LOG_ERROR("lock mutex error: 0x%08llX.", returnCode);
        return returnCode;
    }

    *burstCount = s_cameraBurstCount;

    returnCode = osalHandler->MutexUnlock(s_commonMutex);
    if (returnCode != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
        USER_LOG_ERROR("unlock mutex error: 0x%08llX.", returnCode);
        return returnCode;
    }

    return DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS;
}
T_DjiReturnCode SetPhotoTimeIntervalSettings(T_DjiCameraPhotoTimeIntervalSettings settings){
    T_DjiReturnCode returnCode;
    T_DjiOsalHandler *osalHandler = DjiPlatform_GetOsalHandler();

    returnCode = osalHandler->MutexLock(s_commonMutex);
    if (returnCode != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
        USER_LOG_ERROR("lock mutex error: 0x%08llX.", returnCode);
        return returnCode;
    }

    s_cameraPhotoTimeIntervalSettings.captureCount = settings.captureCount;
    s_cameraPhotoTimeIntervalSettings.timeIntervalSeconds = settings.timeIntervalSeconds;
    USER_LOG_INFO("set photo interval settings count:%d seconds:%d", settings.captureCount,
                  settings.timeIntervalSeconds);
    s_cameraState.currentPhotoShootingIntervalCount = settings.captureCount;

    returnCode = osalHandler->MutexUnlock(s_commonMutex);
    if (returnCode != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
        USER_LOG_ERROR("unlock mutex error: 0x%08llX.", returnCode);
        return returnCode;
    }

    return DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS;
}
T_DjiReturnCode GetPhotoTimeIntervalSettings(T_DjiCameraPhotoTimeIntervalSettings *settings){
    return DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS;
}
T_DjiReturnCode GetSDCardState(T_DjiCameraSDCardState *sdCardState)
{
    T_DjiReturnCode returnCode;
    T_DjiOsalHandler *osalHandler = DjiPlatform_GetOsalHandler();

    returnCode = osalHandler->MutexLock(s_commonMutex);
    if (returnCode != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
        USER_LOG_ERROR("lock mutex error: 0x%08llX.", returnCode);
        return returnCode;
    }

    memcpy(sdCardState, &s_cameraSDCardState, sizeof(T_DjiCameraSDCardState));

    returnCode = osalHandler->MutexUnlock(s_commonMutex);
    if (returnCode != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
        USER_LOG_ERROR("unlock mutex error: 0x%08llX.", returnCode);
        return returnCode;
    }

    return DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS;
}
T_DjiReturnCode FormatSDCard(void){
    return DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS;
}
T_DjiReturnCode DjiTest_CameraGetMode(E_DjiCameraMode *mode){
        T_DjiReturnCode returnCode;
    T_DjiOsalHandler *osalHandler = DjiPlatform_GetOsalHandler();

    returnCode = osalHandler->MutexLock(s_commonMutex);
    if (returnCode != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
        USER_LOG_ERROR("lock mutex error: 0x%08llX.", returnCode);
        return returnCode;
    }

    *mode = s_cameraState.cameraMode;

    returnCode = osalHandler->MutexUnlock(s_commonMutex);
    if (returnCode != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
        USER_LOG_ERROR("unlock mutex error: 0x%08llX.", returnCode);
        return returnCode;
    }

    return DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS;
    
}


T_DjiReturnCode GetMediaFileDir(char *dirPath)
{
    T_DjiReturnCode returnCode;
    char curFileDirPath[DJI_FILE_PATH_SIZE_MAX];
    char tempPath[DJI_FILE_PATH_SIZE_MAX];

    returnCode = DjiUserUtil_GetCurrentFileDirPath(__FILE__, DJI_FILE_PATH_SIZE_MAX, curFileDirPath);
    if (returnCode != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
        USER_LOG_ERROR("Get file current path error, stat = 0x%08llX", returnCode);
        return returnCode;
    }
    
    snprintf(dirPath, DJI_FILE_PATH_SIZE_MAX, "%sjpg", JPG_DIR);

    return DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS;
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
T_DjiReturnCode GetMediaFileOriginData(const char *filePath, uint32_t offset, uint32_t length, uint8_t *data)
{
    T_DjiReturnCode returnCode;
    uint32_t realLen = 0;
    T_DjiMediaFileHandle mediaFileHandle;

    returnCode = DjiMediaFile_CreateHandle(filePath, &mediaFileHandle);
    if (returnCode != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
        USER_LOG_ERROR("Media file create handle error stat:0x%08llX", returnCode);
        return returnCode;
    }

    returnCode = DjiMediaFile_GetDataOrg(mediaFileHandle, offset, length, data, &realLen);
    if (returnCode != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
        USER_LOG_ERROR("Media file get data error stat:0x%08llX", returnCode);
        return returnCode;
    }

    returnCode = DjiMediaFile_DestroyHandle(mediaFileHandle);
    if (returnCode != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
        USER_LOG_ERROR("Media file destroy handle error stat:0x%08llX", returnCode);
        return returnCode;
    }

    return DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS;
}
T_DjiReturnCode CreateMediaFileThumbNail(const char *filePath)
{
    T_DjiReturnCode returnCode;

    returnCode = DjiMediaFile_CreateHandle(filePath, &s_mediaFileThumbNailHandle);
    if (returnCode != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
        USER_LOG_ERROR("Media file create handle error stat:0x%08llX", returnCode);
        return returnCode;
    }

    returnCode = DjiMediaFile_CreateThm(s_mediaFileThumbNailHandle);
    if (returnCode != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
        USER_LOG_ERROR("Media file create thumb nail error stat:0x%08llX", returnCode);
        return returnCode;
    }

    return DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS;
}
T_DjiReturnCode GetMediaFileThumbNailInfo(const char *filePath, T_DjiCameraMediaFileInfo *fileInfo)
{
    T_DjiReturnCode returnCode;

    USER_UTIL_UNUSED(filePath);

    if (s_mediaFileThumbNailHandle == NULL) {
        USER_LOG_ERROR("Media file thumb nail handle null error");
        return DJI_ERROR_SYSTEM_MODULE_CODE_INVALID_PARAMETER;
    }

    returnCode = DjiMediaFile_GetMediaFileType(s_mediaFileThumbNailHandle, &fileInfo->type);
    if (returnCode != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
        USER_LOG_ERROR("Media file get type error stat:0x%08llX", returnCode);
        return returnCode;
    }

    returnCode = DjiMediaFile_GetMediaFileAttr(s_mediaFileThumbNailHandle, &fileInfo->mediaFileAttr);
    if (returnCode != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
        USER_LOG_ERROR("Media file get attr error stat:0x%08llX", returnCode);
        return returnCode;
    }

    returnCode = DjiMediaFile_GetFileSizeThm(s_mediaFileThumbNailHandle, &fileInfo->fileSize);
    if (returnCode != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
        USER_LOG_ERROR("Media file get size error stat:0x%08llX", returnCode);
        return returnCode;
    }

    return DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS;
}
T_DjiReturnCode GetMediaFileThumbNailData(const char *filePath, uint32_t offset, uint32_t length, uint8_t *data)
{
    T_DjiReturnCode returnCode;
    uint16_t realLen = 0;

    USER_UTIL_UNUSED(filePath);

    if (s_mediaFileThumbNailHandle == NULL) {
        USER_LOG_ERROR("Media file thumb nail handle null error");
        return DJI_ERROR_SYSTEM_MODULE_CODE_INVALID_PARAMETER;
    }

    returnCode = DjiMediaFile_GetDataThm(s_mediaFileThumbNailHandle, offset, length, data, &realLen);
    if (returnCode != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
        USER_LOG_ERROR("Media file get data error stat:0x%08llX", returnCode);
        return returnCode;
    }

    return DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS;
}
T_DjiReturnCode DestroyMediaFileThumbNail(const char *filePath)
{
    T_DjiReturnCode returnCode;

    USER_UTIL_UNUSED(filePath);

    if (s_mediaFileThumbNailHandle == NULL) {
        USER_LOG_ERROR("Media file thumb nail handle null error");
        return DJI_ERROR_SYSTEM_MODULE_CODE_INVALID_PARAMETER;
    }

    returnCode = DjiMediaFile_DestoryThm(s_mediaFileThumbNailHandle);
    if (returnCode != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
        USER_LOG_ERROR("Media file destroy thumb nail error stat:0x%08llX", returnCode);
        return returnCode;
    }

    returnCode = DjiMediaFile_DestroyHandle(s_mediaFileThumbNailHandle);
    if (returnCode != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
        USER_LOG_ERROR("Media file destroy handle error stat:0x%08llX", returnCode);
        return returnCode;
    }

    return DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS;
}
T_DjiReturnCode CreateMediaFileScreenNail(const char *filePath)
{
    T_DjiReturnCode returnCode;

    returnCode = DjiMediaFile_CreateHandle(filePath, &s_mediaFileScreenNailHandle);
    if (returnCode != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
        USER_LOG_ERROR("Media file create handle error stat:0x%08llX", returnCode);
        return returnCode;
    }

    returnCode = DjiMediaFile_CreateScr(s_mediaFileScreenNailHandle);
    if (returnCode != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
        USER_LOG_ERROR("Media file create screen nail error stat:0x%08llX", returnCode);
        return returnCode;
    }

    return DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS;
}
T_DjiReturnCode GetMediaFileScreenNailInfo(const char *filePath, T_DjiCameraMediaFileInfo *fileInfo)
{
    T_DjiReturnCode returnCode;

    USER_UTIL_UNUSED(filePath);

    if (s_mediaFileScreenNailHandle == NULL) {
        USER_LOG_ERROR("Media file screen nail handle null error");
        return DJI_ERROR_SYSTEM_MODULE_CODE_INVALID_PARAMETER;
    }

    returnCode = DjiMediaFile_GetMediaFileType(s_mediaFileScreenNailHandle, &fileInfo->type);
    if (returnCode != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
        USER_LOG_ERROR("Media file get type error stat:0x%08llX", returnCode);
        return returnCode;
    }

    returnCode = DjiMediaFile_GetMediaFileAttr(s_mediaFileScreenNailHandle, &fileInfo->mediaFileAttr);
    if (returnCode != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
        USER_LOG_ERROR("Media file get attr error stat:0x%08llX", returnCode);
        return returnCode;
    }

    returnCode = DjiMediaFile_GetFileSizeScr(s_mediaFileScreenNailHandle, &fileInfo->fileSize);
    if (returnCode != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
        USER_LOG_ERROR("Media file get size error stat:0x%08llX", returnCode);
        return returnCode;
    }

    return DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS;
}
T_DjiReturnCode GetMediaFileScreenNailData(const char *filePath, uint32_t offset, uint32_t length,uint8_t *data)
{
    T_DjiReturnCode returnCode;
    uint16_t realLen = 0;

    USER_UTIL_UNUSED(filePath);

    if (s_mediaFileScreenNailHandle == NULL) {
        USER_LOG_ERROR("Media file screen nail handle null error");
        return DJI_ERROR_SYSTEM_MODULE_CODE_INVALID_PARAMETER;
    }

    returnCode = DjiMediaFile_GetDataScr(s_mediaFileScreenNailHandle, offset, length, data, &realLen);
    if (returnCode != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
        USER_LOG_ERROR("Media file get size error stat:0x%08llX", returnCode);
        return returnCode;
    }

    return DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS;
}
T_DjiReturnCode DestroyMediaFileScreenNail(const char *filePath)
{
    T_DjiReturnCode returnCode;

    USER_UTIL_UNUSED(filePath);

    if (s_mediaFileScreenNailHandle == NULL) {
        USER_LOG_ERROR("Media file screen nail handle null error");
        return DJI_ERROR_SYSTEM_MODULE_CODE_INVALID_PARAMETER;
    }

    returnCode = DjiMediaFile_DestroyScr(s_mediaFileScreenNailHandle);
    if (returnCode != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
        USER_LOG_ERROR("Media file destroy screen nail error stat:0x%08llX", returnCode);
        return returnCode;
    }

    returnCode = DjiMediaFile_DestroyHandle(s_mediaFileScreenNailHandle);
    if (returnCode != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
        USER_LOG_ERROR("Media file destroy handle error stat:0x%08llX", returnCode);
        return returnCode;
    }

    return DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS;
}
T_DjiReturnCode DeleteMediaFile(char *filePath)
{
    T_DjiReturnCode returnCode;

    USER_LOG_INFO("delete media file:%s", filePath);
    returnCode = DjiFile_Delete(filePath);
    if (returnCode != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
        USER_LOG_ERROR("Media file delete error stat:0x%08llX", returnCode);
        return returnCode;
    }

    return DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS;
}
T_DjiReturnCode SetMediaPlaybackFile(const char *filePath)
{
    USER_LOG_INFO("set media playback file:%s", filePath);
    T_DjiReturnCode returnCode;

    returnCode = DjiPlayback_StopPlay(&s_playbackInfo);
    if (returnCode != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
        return returnCode;
    }

    returnCode = DjiPlayback_SetPlayFile(&s_playbackInfo, filePath, 0);
    if (returnCode != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
        return returnCode;
    }

    returnCode = DjiPlayback_StartPlay(&s_playbackInfo);
    if (returnCode != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
        return returnCode;
    }

    return DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS;
}
T_DjiReturnCode StartMediaPlayback(void)
{
    T_DjiReturnCode returnCode;

    USER_LOG_INFO("start media playback");
    returnCode = DjiPlayback_StartPlay(&s_playbackInfo);
    if (returnCode != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
        USER_LOG_ERROR("start media playback status error, stat:0x%08llX", returnCode);
        return returnCode;
    }

    return returnCode;
}
T_DjiReturnCode StopMediaPlayback(void)
{
    T_DjiReturnCode returnCode;

    USER_LOG_INFO("stop media playback");
    returnCode = DjiPlayback_StopPlay(&s_playbackInfo);
    if (returnCode != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
        USER_LOG_ERROR("stop media playback error, stat:0x%08llX", returnCode);
        return returnCode;
    }

    return returnCode;
}
T_DjiReturnCode PauseMediaPlayback(void)
{
    T_DjiReturnCode returnCode;

    USER_LOG_INFO("pause media playback");
    returnCode = DjiPlayback_PausePlay(&s_playbackInfo);
    if (returnCode != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
        USER_LOG_ERROR("pause media playback error, stat:0x%08llX", returnCode);
        return returnCode;
    }

    return returnCode;
}
T_DjiReturnCode SeekMediaPlayback(uint32_t playbackPosition)
{
    T_DjiReturnCode returnCode;

    USER_LOG_INFO("seek media playback:%d", playbackPosition);
    returnCode = DjiPlayback_SeekPlay(&s_playbackInfo, playbackPosition);
    if (returnCode != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
        USER_LOG_ERROR("seek media playback error, stat:0x%08llX", returnCode);
        return returnCode;
    }

    return returnCode;
}
T_DjiReturnCode GetMediaPlaybackStatus(T_DjiCameraPlaybackStatus *status)
{
    T_DjiReturnCode returnCode;

    returnCode = DjiPlayback_GetPlaybackStatus(&s_playbackInfo, status);
    if (returnCode != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
        USER_LOG_ERROR("get playback status error, stat:0x%08llX", returnCode);
        return returnCode;
    }

    status->videoPlayProcess = (uint8_t) (((float) s_playbackInfo.playPosMs / (float) s_playbackInfo.videoLengthMs) *
                                          100);

    USER_LOG_DEBUG("get media playback status %d %d %d %d", status->videoPlayProcess, status->playPosMs,
                   status->videoLengthMs, status->playbackMode);

    return DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS;
}
T_DjiReturnCode StartDownloadNotification(void)
{
    T_DjiReturnCode returnCode;
    T_DjiDataChannelBandwidthProportionOfHighspeedChannel bandwidthProportion = {0};

    USER_LOG_DEBUG("media download start notification.");

    bandwidthProportion.dataStream = 0;
    bandwidthProportion.videoStream = 0;
    bandwidthProportion.downloadStream = 100;

    returnCode = DjiHighSpeedDataChannel_SetBandwidthProportion(bandwidthProportion);
    if (returnCode != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
        USER_LOG_ERROR("Set bandwidth proportion for high speed channel error, stat:0x%08llX.", returnCode);
        return returnCode;
    }

    return DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS;
}
T_DjiReturnCode StopDownloadNotification(void)
{
    T_DjiReturnCode returnCode;
    T_DjiDataChannelBandwidthProportionOfHighspeedChannel bandwidthProportion = {0};

    USER_LOG_DEBUG("media download stop notification.");

    bandwidthProportion.dataStream = 10;
    bandwidthProportion.videoStream = 60;
    bandwidthProportion.downloadStream = 30;

    returnCode = DjiHighSpeedDataChannel_SetBandwidthProportion(bandwidthProportion);
    if (returnCode != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
        USER_LOG_ERROR("Set bandwidth proportion for high speed channel error, stat:0x%08llX.", returnCode);
        return returnCode;
    }

    return DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS;
}
T_DjiReturnCode DjiPlayback_StopPlay(T_DjiPlaybackInfo *playbackInfo)
{
    T_DjiReturnCode returnCode;

    returnCode = DjiPlayback_StopPlayProcess();
    if (returnCode != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
        USER_LOG_ERROR("stop play error ");
    }

    playbackInfo->isInPlayProcess = 0;
    playbackInfo->playPosMs = 0;

    return returnCode;
}
T_DjiReturnCode DjiPlayback_PausePlay(T_DjiPlaybackInfo *playbackInfo)
{
    T_DjiReturnCode returnCode = DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS;
    T_DjiOsalHandler *osalHandler = DjiPlatform_GetOsalHandler();

    T_TestPayloadCameraPlaybackCommand playbackCommand ;
    if (playbackInfo->isInPlayProcess) {
        playbackCommand.command = TEST_PAYLOAD_CAMERA_MEDIA_PLAY_COMMAND_PAUSE;

        if (osalHandler->MutexLock(s_mediaPlayCommandBufferMutex) != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
            USER_LOG_ERROR("mutex lock error");
            return DJI_ERROR_SYSTEM_MODULE_CODE_UNKNOWN;
        }

        if (UtilBuffer_GetUnusedSize(&s_mediaPlayCommandBufferHandler) >= sizeof(T_TestPayloadCameraPlaybackCommand)) {
            UtilBuffer_Put(&s_mediaPlayCommandBufferHandler, (const uint8_t *) &playbackCommand,
                           sizeof(T_TestPayloadCameraPlaybackCommand));
        } else {
            USER_LOG_ERROR("Media playback command buffer is full.");
            returnCode = DJI_ERROR_SYSTEM_MODULE_CODE_OUT_OF_RANGE;
        }

        if (osalHandler->MutexUnlock(s_mediaPlayCommandBufferMutex) != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
            USER_LOG_ERROR("mutex unlock error");
            return DJI_ERROR_SYSTEM_MODULE_CODE_UNKNOWN;
        }
        osalHandler->SemaphorePost(s_mediaPlayWorkSem);
    }

    playbackInfo->isInPlayProcess = 0;

    return returnCode;
}
T_DjiReturnCode DjiPlayback_SetPlayFile(T_DjiPlaybackInfo *playbackInfo, const char *filePath, uint16_t index)
{
    T_DjiReturnCode returnCode;

    if (strlen(filePath) > DJI_FILE_PATH_SIZE_MAX) {
        USER_LOG_ERROR("Dji playback file path out of length range error\n");
        return DJI_ERROR_SYSTEM_MODULE_CODE_INVALID_PARAMETER;
    }

    strcpy(playbackInfo->filePath, filePath);
    playbackInfo->videoIndex = index;

    returnCode = DjiPlayback_GetVideoLengthMs(filePath, &playbackInfo->videoLengthMs);

    if (returnCode != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
        return DJI_ERROR_SYSTEM_MODULE_CODE_UNKNOWN;
    }

    return DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS;
}
T_DjiReturnCode DjiPlayback_SeekPlay(T_DjiPlaybackInfo *playbackInfo, uint32_t seekPos)
{
    T_DjiRunTimeStamps ti;
    T_DjiReturnCode returnCode;

    returnCode = DjiPlayback_PausePlay(playbackInfo);
    if (returnCode != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
        USER_LOG_ERROR("pause play error \n");
        return returnCode;
    }

    playbackInfo->playPosMs = seekPos;
    returnCode = DjiPlayback_StartPlayProcess(playbackInfo->filePath, playbackInfo->playPosMs);
    if (returnCode != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
        USER_LOG_ERROR("start playback process error \n");
        return returnCode;
    }

    playbackInfo->isInPlayProcess = 1;
    ti = DjiUtilTime_GetRunTimeStamps();
    playbackInfo->startPlayTimestampsUs = ti.realUsec - playbackInfo->playPosMs * 1000;

    return DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS;
}
T_DjiReturnCode DjiPlayback_StartPlay(T_DjiPlaybackInfo *playbackInfo)
{
    T_DjiRunTimeStamps ti;
    T_DjiReturnCode returnCode;

    if (playbackInfo->isInPlayProcess == 1) {
        //already in playing, return DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS
        return DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS;
    }

    returnCode = DjiPlayback_StartPlayProcess(playbackInfo->filePath, playbackInfo->playPosMs);
    if (returnCode != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
        USER_LOG_ERROR("start play process error \n");
        return returnCode;
    }

    playbackInfo->isInPlayProcess = 1;

    ti = DjiUtilTime_GetRunTimeStamps();
    playbackInfo->startPlayTimestampsUs = ti.realUsec - playbackInfo->playPosMs * 1000;

    return DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS;
}
T_DjiReturnCode DjiPlayback_GetPlaybackStatus(T_DjiPlaybackInfo *playbackInfo,T_DjiCameraPlaybackStatus *playbackStatus)
{
    T_DjiRunTimeStamps timeStamps;

    memset(playbackStatus, 0, sizeof(T_DjiCameraPlaybackStatus));

    //update playback pos info
    if (playbackInfo->isInPlayProcess) {
        timeStamps = DjiUtilTime_GetRunTimeStamps();
        playbackInfo->playPosMs = (timeStamps.realUsec - playbackInfo->startPlayTimestampsUs) / 1000;

        if (playbackInfo->playPosMs >= playbackInfo->videoLengthMs) {
            if (DjiPlayback_PausePlay(playbackInfo) != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
                return DJI_ERROR_SYSTEM_MODULE_CODE_UNKNOWN;
            }
        }
    }

    //set playback status
    if (playbackInfo->isInPlayProcess == 0 && playbackInfo->playPosMs != 0) {
        playbackStatus->playbackMode = DJI_CAMERA_PLAYBACK_MODE_PAUSE;
    } else if (playbackInfo->isInPlayProcess) {
        playbackStatus->playbackMode = DJI_CAMERA_PLAYBACK_MODE_PLAY;
    } else {
        playbackStatus->playbackMode = DJI_CAMERA_PLAYBACK_MODE_STOP;
    }

    playbackStatus->playPosMs = playbackInfo->playPosMs;
    playbackStatus->videoLengthMs = playbackInfo->videoLengthMs;

    if (playbackInfo->videoLengthMs != 0) {
        playbackStatus->videoPlayProcess =
            (playbackInfo->videoLengthMs - playbackInfo->playPosMs) / playbackInfo->videoLengthMs;
    } else {
        playbackStatus->videoPlayProcess = 0;
    }

    return DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS;
}
T_DjiReturnCode DjiPlayback_StopPlayProcess(void)
{
    T_DjiReturnCode returnCode = DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS;
    T_TestPayloadCameraPlaybackCommand playbackCommand ;
    T_DjiOsalHandler *osalHandler = DjiPlatform_GetOsalHandler();

    playbackCommand.command = TEST_PAYLOAD_CAMERA_MEDIA_PLAY_COMMAND_STOP;

    if (osalHandler->MutexLock(s_mediaPlayCommandBufferMutex) != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
        USER_LOG_ERROR("mutex lock error");
        return DJI_ERROR_SYSTEM_MODULE_CODE_UNKNOWN;
    }

    if (UtilBuffer_GetUnusedSize(&s_mediaPlayCommandBufferHandler) >= sizeof(T_TestPayloadCameraPlaybackCommand)) {
        UtilBuffer_Put(&s_mediaPlayCommandBufferHandler, (const uint8_t *) &playbackCommand,
                       sizeof(T_TestPayloadCameraPlaybackCommand));
    } else {
        USER_LOG_ERROR("Media playback command buffer is full.");
        returnCode = DJI_ERROR_SYSTEM_MODULE_CODE_OUT_OF_RANGE;
    }

    if (osalHandler->MutexUnlock(s_mediaPlayCommandBufferMutex) != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
        USER_LOG_ERROR("mutex unlock error");
        return DJI_ERROR_SYSTEM_MODULE_CODE_UNKNOWN;
    }
    osalHandler->SemaphorePost(s_mediaPlayWorkSem);
    return returnCode;
}
T_DjiReturnCode DjiPlayback_GetVideoLengthMs(const char *filePath, uint32_t *videoLengthMs)
{
    FILE *fp;
    T_DjiReturnCode returnCode;
    char ffmpegCmdStr[FFMPEG_CMD_BUF_SIZE];
    float hour, minute, second;
    char tempTailStr[128];
    int ret;

    snprintf(ffmpegCmdStr, FFMPEG_CMD_BUF_SIZE, "ffmpeg -i \"%s\" 2>&1 | grep \"Duration\"", filePath);
    fp = popen(ffmpegCmdStr, "r");

    if (fp == NULL) {
        return DJI_ERROR_SYSTEM_MODULE_CODE_SYSTEM_ERROR;
    }

    ret = fscanf(fp, "  Duration: %f:%f:%f,%127s", &hour, &minute, &second, tempTailStr);
    if (ret <= 0) {
        USER_LOG_ERROR("MP4 File Get Duration Error\n");
        returnCode = DJI_ERROR_SYSTEM_MODULE_CODE_SYSTEM_ERROR;
        goto out;
    }

    *videoLengthMs = (uint32_t) ((hour * 3600 + minute * 60 + second) * 1000);
    returnCode = DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS;

out:
    pclose(fp);

    return returnCode;
}
T_DjiReturnCode DjiPlayback_StartPlayProcess(const char *filePath, uint32_t playPosMs)
{
    T_DjiReturnCode returnCode = DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS;
    T_TestPayloadCameraPlaybackCommand mediaPlayCommand;
    T_DjiOsalHandler *osalHandler = DjiPlatform_GetOsalHandler();

    mediaPlayCommand.command = TEST_PAYLOAD_CAMERA_MEDIA_PLAY_COMMAND_START;
    mediaPlayCommand.timeMs = playPosMs;

    if (strlen(filePath) >= sizeof(mediaPlayCommand.path)) {
        USER_LOG_ERROR("File path is too long.");
        return DJI_ERROR_SYSTEM_MODULE_CODE_OUT_OF_RANGE;
    }
    memcpy(mediaPlayCommand.path, filePath, strlen(filePath));

    if (osalHandler->MutexLock(s_mediaPlayCommandBufferMutex) != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
        USER_LOG_ERROR("mutex lock error");
        return DJI_ERROR_SYSTEM_MODULE_CODE_UNKNOWN;
    }

    if (UtilBuffer_GetUnusedSize(&s_mediaPlayCommandBufferHandler) >= sizeof(T_TestPayloadCameraPlaybackCommand)) {
        UtilBuffer_Put(&s_mediaPlayCommandBufferHandler, (const uint8_t *) &mediaPlayCommand,
                       sizeof(T_TestPayloadCameraPlaybackCommand));
    } else {
        USER_LOG_ERROR("Media playback command buffer is full.");
        returnCode = DJI_ERROR_SYSTEM_MODULE_CODE_OUT_OF_RANGE;
    }

    if (osalHandler->MutexUnlock(s_mediaPlayCommandBufferMutex) != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
        USER_LOG_ERROR("mutex unlock error");
        return DJI_ERROR_SYSTEM_MODULE_CODE_UNKNOWN;
    }
    osalHandler->SemaphorePost(s_mediaPlayWorkSem);
    return returnCode;
}