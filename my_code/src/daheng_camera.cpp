#include "daheng_camera.h"


static void GX_STDC OnFrameCallbackFun(GX_FRAME_CALLBACK_PARAM* pFrame)
{  
    if (pFrame->status == GX_FRAME_STATUS_SUCCESS)
    {
        int width = pFrame->nWidth;
        int height = pFrame->nHeight;
        cv::Mat* outputImage = static_cast<cv::Mat*>(pFrame->pUserParam);
        *outputImage = cv::Mat(pFrame->nHeight, pFrame->nWidth, CV_8UC1, const_cast<void*>(pFrame->pImgBuf));
    }
};
daheng_camera::daheng_camera(){}

bool daheng_camera::initialize() {
    
    status = GXInitLib(); 
   
    if (status != GX_STATUS_SUCCESS) 
    { 
        return -1; 
    }
    status = GXUpdateDeviceList(&nDeviceNum, 1000);
    if ((status != GX_STATUS_SUCCESS) || (nDeviceNum <= 0))
    {
       return -1;
    }
    //cap.set(cv::CAP_PROP_FOURCC, cv::VideoWriter::fourcc('Y', 'U', 'Y', 'V'));
    return 1;
}

void daheng_camera::open() {
    stOpenParam.accessMode = GX_ACCESS_EXCLUSIVE;
    stOpenParam.openMode = GX_OPEN_INDEX;
    stOpenParam.pszContent = const_cast<char*>("1");
    status = GXOpenDevice(&stOpenParam, &hDevice);
    status = GXRegisterCaptureCallback(hDevice, &image, OnFrameCallbackFun);
    
}

void daheng_camera::close() {
    status = GXUnregisterCaptureCallback(hDevice);
    status = GXCloseDevice(hDevice);
    status = GXCloseLib(); 
}
void daheng_camera::stream_off()
{       status = GXSendCommand(hDevice, GX_COMMAND_ACQUISITION_STOP);
        }
void daheng_camera::stream_on()
{
    status = GXSetEnum(hDevice, GX_ENUM_EXPOSURE_MODE, GX_EXPOSURE_MODE_TIMED );
    GX_FLOAT_RANGE shutterRange;
    
    status = GXGetFloatRange(hDevice, GX_FLOAT_EXPOSURE_TIME,  &shutterRange);
    double exposuretime=20000;
    status = GXSetFloat(hDevice, GX_FLOAT_EXPOSURE_TIME,exposuretime);
    status = GXSendCommand(hDevice, GX_COMMAND_ACQUISITION_START);}
cv::Mat daheng_camera::getCurrentFrame() 
{ 
    return image;
}
void daheng_camera::set_exp(double extime)
{
    status = GXSetFloat(hDevice, GX_FLOAT_EXPOSURE_TIME,extime);
}
