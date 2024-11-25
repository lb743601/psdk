#ifndef PSDK_INIT_H
#define PSDK_INIT_H
//--------Include--------
#include<iostream>
#include "dji_typedef.h"
#include "dji_core.h"
#include "dji_logger.h"
#include "dji_platform.h"
#include "hal_network.h"
#include "hal_uart.h"
#include "osal_socket.h"
#include "osal.h"
#include "osal_fs.h"
#include "dji_aircraft_info.h"
#include <csignal>
#include "camera_emu/dji_media_file_manage/dji_media_file_core.h"
#include "dji_payload_camera.h"
#include <jpeglib.h>
#include <algorithm>
#include <cmath>
#include <chrono>
#include <iomanip>
#include <sys/stat.h>
#include <fstream>
#include <sstream>
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
//--------PSDK_Info------
#define USER_APP_NAME               "TEST"
#define USER_APP_ID                 "154375"
#define USER_APP_KEY                "0aa926cbf65db6ddf6f442bd273c651"
#define USER_APP_LICENSE            "PrxHQ3wAXren6BIDXO5SBM3neka3wp8xo+8yD1IpZU/69w2O3EQiijAjEP1OhGN44v/wV9UAl5dgSwlh4z/SLiqu1Iyw55yFWQ+DAeZ6w7kF+ywfe6WUrHZOFN7/sVLbPFxK+xf57SB3PmE29YT2vH48/Q7SfTAWudD1IgwrpaRpk6D5kbhxTkSwLCPLHUFXz7Ff88464ilRoGVjtQWTP+1t4O3rDitLB1hp4vjvhfpJ3fzr+F55BKx0EvX5MFYYxXQkTVC4PVD0j90VICi1wqr11LKI0kkYmissfQZuMx2X2skQ4jK+slxu+EOlRy5UJ43GngPPYCZSbp2ZZmohQQ=="
#define USER_DEVELOPER_ACCOUNT      "17718630648@163.com"
#define USER_BAUD_RATE              "460800"
//--------Define---------

#define JPG_DIR                     "/home/jetson/Downloads/PSDK_V2/my_psdk/build/"
#define INTERVAL_PHOTOGRAPH_ALWAYS_COUNT        (255)
#define INTERVAL_PHOTOGRAPH_INTERVAL_INIT_VALUE (1) 
#define TAKING_PHOTO_SPENT_TIME_MS_EMU          (500)
#define PAYLOAD_CAMERA_EMU_TASK_FREQ            (100)
#define FFMPEG_CMD_BUF_SIZE                 (256 + 256)
#define DJI_LOG_PATH  "Logs/DJI"
#define DJI_LOG_PATH                    "Logs/DJI"
#define DJI_LOG_INDEX_FILE_NAME         "Logs/latest"
#define DJI_LOG_FOLDER_NAME             "Logs"
#define DJI_LOG_PATH_MAX_SIZE           (128)
#define DJI_LOG_FOLDER_NAME_MAX_SIZE    (32)
#define DJI_SYSTEM_CMD_STR_MAX_SIZE     (64)
#define DJI_LOG_MAX_COUNT               (10)
#define SDCARD_TOTAL_SPACE_IN_MB                (32 * 1024)
#define SDCARD_PER_PHOTO_SPACE_IN_MB            (4)
#define SDCARD_PER_SECONDS_RECORD_SPACE_IN_MB   (2)
#define USER_UTIL_UNUSED(x)                                 ((x) = (x))
#define USER_UTIL_MIN(a, b)                                 (((a) < (b)) ? (a) : (b))
#define USER_UTIL_MAX(a, b)                                 (((a) > (b)) ? (a) : (b))
//--------Declaration----

//--------Struct---------
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

typedef struct {
    uint8_t isInPlayProcess;
    uint16_t videoIndex;
    char filePath[DJI_FILE_PATH_SIZE_MAX];
    uint32_t videoLengthMs;
    uint64_t startPlayTimestampsUs;
    uint64_t playPosMs;
} T_DjiPlaybackInfo;
//--------variable-------
static FILE *s_djiLogFile;
static FILE *s_djiLogFileCnt;
static T_DjiMutexHandle s_commonMutex;
static T_DjiMutexHandle s_zoomMutex;
static T_DjiMutexHandle s_tapZoomMutex;
static T_DjiCameraCommonHandler s_commonHandler;
static T_DjiCameraMediaDownloadPlaybackHandler s_psdkCameraMedia;
static T_DjiCameraSDCardState s_cameraSDCardState;
static T_DjiCameraSystemState s_cameraState ;
static T_UtilBuffer s_mediaPlayCommandBufferHandler;
static uint8_t s_mediaPlayCommandBuffer[sizeof(T_TestPayloadCameraPlaybackCommand) * 32];
static T_DjiMutexHandle s_mediaPlayCommandBufferMutex ;
static T_DjiSemaHandle s_mediaPlayWorkSem;
static T_DjiTaskHandle s_userSendVideoThread;
static T_DjiMediaFileHandle s_mediaFileThumbNailHandle;
static T_DjiMediaFileHandle s_mediaFileScreenNailHandle;
static T_DjiPlaybackInfo s_playbackInfo;
static E_DjiCameraShootPhotoMode s_cameraShootPhotoMode;
static E_DjiCameraBurstCount s_cameraBurstCount = DJI_CAMERA_BURST_COUNT_2;
static T_DjiCameraPhotoTimeIntervalSettings s_cameraPhotoTimeIntervalSettings = {INTERVAL_PHOTOGRAPH_ALWAYS_COUNT,INTERVAL_PHOTOGRAPH_INTERVAL_INIT_VALUE};


//--------public---------
T_DjiReturnCode SetEnvironment(void);
T_DjiReturnCode StartApp(void);
T_DjiReturnCode SetCameraEnvironment(void);
T_DjiReturnCode Camera_Init(void);
//--------private--------
T_DjiReturnCode DjiUser_PrintConsole(const uint8_t *data, uint16_t dataLen);
T_DjiReturnCode DjiUser_LocalWriteFsInit(const char *path);
static void *UserCameraMedia_SendVideoTask(void *arg);

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
static void DjiUser_NormalExitHandler(int signalNum);
void media_cleanup();
T_DjiReturnCode DjiTest_CameraGetMode(E_DjiCameraMode *mode);
static T_DjiReturnCode GetMediaFileDir(char *dirPath);
static T_DjiReturnCode GetMediaFileOriginData(const char *filePath, uint32_t offset, uint32_t length,uint8_t *data);
static T_DjiReturnCode CreateMediaFileThumbNail(const char *filePath);
static T_DjiReturnCode GetMediaFileThumbNailInfo(const char *filePath, T_DjiCameraMediaFileInfo *fileInfo);
static T_DjiReturnCode GetMediaFileThumbNailData(const char *filePath, uint32_t offset, uint32_t length,uint8_t *data);
static T_DjiReturnCode DestroyMediaFileThumbNail(const char *filePath);
static T_DjiReturnCode CreateMediaFileScreenNail(const char *filePath);
static T_DjiReturnCode GetMediaFileScreenNailInfo(const char *filePath, T_DjiCameraMediaFileInfo *fileInfo);
static T_DjiReturnCode GetMediaFileScreenNailData(const char *filePath, uint32_t offset, uint32_t length,uint8_t *data);
static T_DjiReturnCode DestroyMediaFileScreenNail(const char *filePath);
static T_DjiReturnCode DeleteMediaFile(char *filePath);
static T_DjiReturnCode SetMediaPlaybackFile(const char *filePath);
static T_DjiReturnCode StartMediaPlayback(void);
static T_DjiReturnCode StopMediaPlayback(void);
static T_DjiReturnCode PauseMediaPlayback(void);
static T_DjiReturnCode SeekMediaPlayback(uint32_t playbackPosition);
static T_DjiReturnCode GetMediaPlaybackStatus(T_DjiCameraPlaybackStatus *status);
static T_DjiReturnCode StartDownloadNotification(void);
static T_DjiReturnCode StopDownloadNotification(void);
static void *UserCameraMedia_SendVideoTask(void *arg);
static T_DjiReturnCode DjiPlayback_GetVideoLengthMs(const char *filePath, uint32_t *videoLengthMs);
static T_DjiReturnCode DjiPlayback_StartPlayProcess(const char *filePath, uint32_t playPosMs);
static T_DjiReturnCode DjiPlayback_StopPlayProcess(void);
static T_DjiReturnCode DjiPlayback_StopPlayProcess(void);
static T_DjiReturnCode DjiPlayback_StopPlay(T_DjiPlaybackInfo *playbackInfo);
static T_DjiReturnCode DjiPlayback_PausePlay(T_DjiPlaybackInfo *playbackInfo);
static T_DjiReturnCode DjiPlayback_SetPlayFile(T_DjiPlaybackInfo *playbackInfo, const char *filePath,uint16_t index);
static T_DjiReturnCode DjiPlayback_SeekPlay(T_DjiPlaybackInfo *playbackInfo, uint32_t seekPos);
static T_DjiReturnCode DjiPlayback_StartPlay(T_DjiPlaybackInfo *playbackInfo);
static T_DjiReturnCode DjiPlayback_GetPlaybackStatus(T_DjiPlaybackInfo *playbackInfo,T_DjiCameraPlaybackStatus *playbackStatus);
static T_DjiReturnCode DjiUser_LocalWrite(const uint8_t *data, uint16_t dataLen);
static T_DjiReturnCode DjiTest_CameraMediaGetFileInfo(const char *filePath, T_DjiCameraMediaFileInfo *fileInfo);
#endif
