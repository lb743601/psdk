#ifndef INIT_H
#define INIT_H
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
#define FFMPEG_CMD_BUF_SIZE                 (256 + 256)
#define DJI_LOG_PATH  "Logs/DJI"
#define DJI_LOG_PATH                    "Logs/DJI"
#define DJI_LOG_INDEX_FILE_NAME         "Logs/latest"
#define DJI_LOG_FOLDER_NAME             "Logs"
#define DJI_LOG_PATH_MAX_SIZE           (128)
#define DJI_LOG_FOLDER_NAME_MAX_SIZE    (32)
#define DJI_SYSTEM_CMD_STR_MAX_SIZE     (64)
#define DJI_LOG_MAX_COUNT               (10)
#define USER_APP_NAME               "TEST"
#define USER_APP_ID                 "154375"
#define USER_APP_KEY                "0aa926cbf65db6ddf6f442bd273c651"
#define USER_APP_LICENSE            "PrxHQ3wAXren6BIDXO5SBM3neka3wp8xo+8yD1IpZU/69w2O3EQiijAjEP1OhGN44v/wV9UAl5dgSwlh4z/SLiqu1Iyw55yFWQ+DAeZ6w7kF+ywfe6WUrHZOFN7/sVLbPFxK+xf57SB3PmE29YT2vH48/Q7SfTAWudD1IgwrpaRpk6D5kbhxTkSwLCPLHUFXz7Ff88464ilRoGVjtQWTP+1t4O3rDitLB1hp4vjvhfpJ3fzr+F55BKx0EvX5MFYYxXQkTVC4PVD0j90VICi1wqr11LKI0kkYmissfQZuMx2X2skQ4jK+slxu+EOlRy5UJ43GngPPYCZSbp2ZZmohQQ=="
#define USER_DEVELOPER_ACCOUNT      "17718630648@163.com"
#define USER_BAUD_RATE              "460800"
#define SDCARD_TOTAL_SPACE_IN_MB                (32 * 1024)
#define SDCARD_PER_PHOTO_SPACE_IN_MB            (4)
#define SDCARD_PER_SECONDS_RECORD_SPACE_IN_MB   (2)
#define USER_UTIL_UNUSED(x)                                 ((x) = (x))
#define USER_UTIL_MIN(a, b)                                 (((a) < (b)) ? (a) : (b))
#define USER_UTIL_MAX(a, b)                                 (((a) > (b)) ? (a) : (b))
T_DjiReturnCode DjiUser_LocalWrite(const uint8_t *data, uint16_t dataLen);
T_DjiReturnCode DjiUser_PrintConsole(const uint8_t *data, uint16_t dataLen);
T_DjiReturnCode DjiUser_LocalWriteFsInit(const char *path);
T_DjiReturnCode DjiTest_CameraMediaGetFileInfo(const char *filePath, T_DjiCameraMediaFileInfo *fileInfo);
void init();
void start_service();
static void DjiUser_NormalExitHandler(int signalNum);
T_DjiReturnCode DjiTest_CameraEmuBaseStartService(void);
T_DjiReturnCode media_init(void);
void media_cleanup(void);
static FILE *s_djiLogFile;
static FILE *s_djiLogFileCnt;

#endif