#ifndef DAHENG_CAMERA_H
#define DAHENG_CAMERA_H

#include "GxIAPI.h" 
#include <iostream>
#include "DxImageProc.h"
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>
#include <opencv2/opencv.hpp>
class daheng_camera {
public:
    daheng_camera();
    bool initialize();
    void open();
    void close();
    void stream_on();
    void stream_off();
    cv::Mat getCurrentFrame();
    void set_exp(double extime);

private:
    GX_STATUS status ;
    GX_DEV_HANDLE hDevice ;
    uint32_t nDeviceNum ;
    PGX_FRAME_BUFFER pFrameBuffer;
    GX_OPEN_PARAM stOpenParam;
    cv::Mat image;
};

#endif // CAMERA_H
