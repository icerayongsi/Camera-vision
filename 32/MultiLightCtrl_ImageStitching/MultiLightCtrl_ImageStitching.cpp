
/*
* 这个示例演示了线阵相机采用2组分时曝光参数，先拆图再自上而下拼图，并将拼好的裸图以bmp格式保存至本地。
* This sample shows the linear array camera uses 2 sets of time-sharing exposure to acquire raw data
* then disassemble the raw data and  top-down puzzle
* and last save to bmp format to local.
*/


#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include "MvCameraControl.h"

bool g_bExit = false;

#define EXPOSURE_NUM 2  // 分时频闪的灯数, 默认曝光个数为2

// ch:等待按键输入 | en:Wait for key press
void PressEnterToExit(void)
{
    int c;
    while ( (c = getchar()) != '\n' && c != EOF );
    fprintf( stderr, "\nPress enter to exit.\n");
    while( getchar() != '\n');
    g_bExit = true;
    sleep(1);
}
    
bool IsHBPixelFormat(MvGvspPixelType ePixelType)
{
    switch (ePixelType)
    {
    case PixelType_Gvsp_HB_Mono8:
    case PixelType_Gvsp_HB_Mono10:
    case PixelType_Gvsp_HB_Mono10_Packed:
    case PixelType_Gvsp_HB_Mono12:
    case PixelType_Gvsp_HB_Mono12_Packed:
    case PixelType_Gvsp_HB_Mono16:
    case PixelType_Gvsp_HB_RGB8_Packed:
    case PixelType_Gvsp_HB_BGR8_Packed:
    case PixelType_Gvsp_HB_RGBA8_Packed:
    case PixelType_Gvsp_HB_BGRA8_Packed:
    case PixelType_Gvsp_HB_RGB16_Packed:
    case PixelType_Gvsp_HB_BGR16_Packed:
    case PixelType_Gvsp_HB_RGBA16_Packed:
    case PixelType_Gvsp_HB_BGRA16_Packed:
    case PixelType_Gvsp_HB_YUV422_Packed:
    case PixelType_Gvsp_HB_YUV422_YUYV_Packed:
    case PixelType_Gvsp_HB_BayerGR8:
    case PixelType_Gvsp_HB_BayerRG8:
    case PixelType_Gvsp_HB_BayerGB8:
    case PixelType_Gvsp_HB_BayerBG8:
    case PixelType_Gvsp_HB_BayerRBGG8:
    case PixelType_Gvsp_HB_BayerGB10:
    case PixelType_Gvsp_HB_BayerGB10_Packed:
    case PixelType_Gvsp_HB_BayerBG10:
    case PixelType_Gvsp_HB_BayerBG10_Packed:
    case PixelType_Gvsp_HB_BayerRG10:
    case PixelType_Gvsp_HB_BayerRG10_Packed:
    case PixelType_Gvsp_HB_BayerGR10:
    case PixelType_Gvsp_HB_BayerGR10_Packed:
    case PixelType_Gvsp_HB_BayerGB12:
    case PixelType_Gvsp_HB_BayerGB12_Packed:
    case PixelType_Gvsp_HB_BayerBG12:
    case PixelType_Gvsp_HB_BayerBG12_Packed:
    case PixelType_Gvsp_HB_BayerRG12:
    case PixelType_Gvsp_HB_BayerRG12_Packed:
    case PixelType_Gvsp_HB_BayerGR12:
    case PixelType_Gvsp_HB_BayerGR12_Packed:
        return true;
    default:
        return false;
    }
}

bool PrintDeviceInfo(MV_CC_DEVICE_INFO* pstMVDevInfo)
{
    if (NULL == pstMVDevInfo)
    {
        printf("The Pointer of pstMVDevInfo is NULL!\n");
        return false;
    }
    if (pstMVDevInfo->nTLayerType == MV_GIGE_DEVICE)
    {
        int nIp1 = ((pstMVDevInfo->SpecialInfo.stGigEInfo.nCurrentIp & 0xff000000) >> 24);
        int nIp2 = ((pstMVDevInfo->SpecialInfo.stGigEInfo.nCurrentIp & 0x00ff0000) >> 16);
        int nIp3 = ((pstMVDevInfo->SpecialInfo.stGigEInfo.nCurrentIp & 0x0000ff00) >> 8);
        int nIp4 = (pstMVDevInfo->SpecialInfo.stGigEInfo.nCurrentIp & 0x000000ff);

        // ch:打印当前相机ip和用户自定义名字 | en:print current ip and user defined name
        printf("CurrentIp: %d.%d.%d.%d\n" , nIp1, nIp2, nIp3, nIp4);
        printf("UserDefinedName: %s\n\n" , pstMVDevInfo->SpecialInfo.stGigEInfo.chUserDefinedName);
    }
    else if (pstMVDevInfo->nTLayerType == MV_USB_DEVICE)
    {
        printf("UserDefinedName: %s\n", pstMVDevInfo->SpecialInfo.stUsb3VInfo.chUserDefinedName);
        printf("Serial Number: %s\n", pstMVDevInfo->SpecialInfo.stUsb3VInfo.chSerialNumber);
        printf("Device Number: %d\n\n", pstMVDevInfo->SpecialInfo.stUsb3VInfo.nDeviceNumber);
    }
    else if (pstMVDevInfo->nTLayerType == MV_GENTL_GIGE_DEVICE)
    {
        printf("UserDefinedName: %s\n", pstMVDevInfo->SpecialInfo.stGigEInfo.chUserDefinedName);
        printf("Serial Number: %s\n", pstMVDevInfo->SpecialInfo.stGigEInfo.chSerialNumber);
        printf("Model Name: %s\n\n", pstMVDevInfo->SpecialInfo.stGigEInfo.chModelName);
    }
    else if (pstMVDevInfo->nTLayerType == MV_GENTL_CAMERALINK_DEVICE)
    {
        printf("UserDefinedName: %s\n", pstMVDevInfo->SpecialInfo.stCMLInfo.chUserDefinedName);
        printf("Serial Number: %s\n", pstMVDevInfo->SpecialInfo.stCMLInfo.chSerialNumber);
        printf("Model Name: %s\n\n", pstMVDevInfo->SpecialInfo.stCMLInfo.chModelName);
    }
    else if (pstMVDevInfo->nTLayerType == MV_GENTL_CXP_DEVICE)
    {
        printf("UserDefinedName: %s\n", pstMVDevInfo->SpecialInfo.stCXPInfo.chUserDefinedName);
        printf("Serial Number: %s\n", pstMVDevInfo->SpecialInfo.stCXPInfo.chSerialNumber);
        printf("Model Name: %s\n\n", pstMVDevInfo->SpecialInfo.stCXPInfo.chModelName);
    }
    else if (pstMVDevInfo->nTLayerType == MV_GENTL_XOF_DEVICE)
    {
        printf("UserDefinedName: %s\n", pstMVDevInfo->SpecialInfo.stXoFInfo.chUserDefinedName);
        printf("Serial Number: %s\n", pstMVDevInfo->SpecialInfo.stXoFInfo.chSerialNumber);
        printf("Model Name: %s\n\n", pstMVDevInfo->SpecialInfo.stXoFInfo.chModelName);
    }
    else
    {
        printf("Not support.\n");
    }

    return true;
}

int main()
{
    int nRet = MV_OK;
    void* handle = NULL;
    unsigned char* pReconstructBuffer = NULL;
    unsigned char * pDstBuf = NULL;

    do 
    {
        // ch:初始化SDK | en:Initialize SDK
        nRet = MV_CC_Initialize();
        if (MV_OK != nRet)
        {
            printf("Initialize SDK fail! nRet [0x%x]\n", nRet);
            break;
        }

        // ch:枚举设备 | en:Enum device
        MV_CC_DEVICE_INFO_LIST stDeviceList;
        memset(&stDeviceList, 0, sizeof(MV_CC_DEVICE_INFO_LIST));
        nRet = MV_CC_EnumDevices(MV_GIGE_DEVICE | MV_USB_DEVICE , &stDeviceList);
        if (MV_OK != nRet)
        {
            printf("Enum Devices fail! nRet [0x%x]\n", nRet);
            break;
        }

        if (stDeviceList.nDeviceNum > 0)
        {
            for (unsigned int i = 0; i < stDeviceList.nDeviceNum; i++)
            {
                printf("[device %d]:\n", i);
                MV_CC_DEVICE_INFO* pDeviceInfo = stDeviceList.pDeviceInfo[i];
                if (NULL == pDeviceInfo)
                {
                    break;
                } 
                PrintDeviceInfo(pDeviceInfo);            
            }  
        } 
        else
        {
            printf("Find No Devices!\n");
            break;
        }

        printf("Please Intput camera index: ");
        unsigned int nIndex = 0;
        scanf("%d", &nIndex);

        if (nIndex >= stDeviceList.nDeviceNum)
        {
            printf("Input error!\n");
            break;
        }

        // ch:选择设备并创建句柄 | en:Select device and create handle
        nRet = MV_CC_CreateHandle(&handle, stDeviceList.pDeviceInfo[nIndex]);
        if (MV_OK != nRet)
        {
            printf("Create Handle fail! nRet [0x%x]\n", nRet);
            break;
        }

        // ch:打开设备 | en:Open device
        nRet = MV_CC_OpenDevice(handle);
        if (MV_OK != nRet)
        {
            printf("Open Device fail! nRet [0x%x]\n", nRet);
            break;
        }

        // ch:探测网络最佳包大小(只对GigE相机有效) | en:Detection network optimal package size(It only works for the GigE camera)
        if (stDeviceList.pDeviceInfo[nIndex]->nTLayerType == MV_GIGE_DEVICE)
        {
            int nPacketSize = MV_CC_GetOptimalPacketSize(handle);
            if (nPacketSize > 0)
            {
                nRet = MV_CC_SetIntValue(handle,"GevSCPSPacketSize",nPacketSize);
                if(nRet != MV_OK)
                {
                    printf("Warning: Set Packet Size fail nRet [0x%x]!", nRet);
                }
            }
            else
            {
                printf("Warning: Get Packet Size fail nRet [0x%x]!", nPacketSize);
            }
        }

        // ch:获取数据包大小 | en:Get payload size
        MVCC_INTVALUE stParam;
        memset(&stParam, 0, sizeof(MVCC_INTVALUE));
        nRet = MV_CC_GetIntValue(handle, "PayloadSize", &stParam);
        if (MV_OK != nRet)
        {
            printf("Get PayloadSize fail! nRet [0x%x]\n", nRet);
            break;
        }
        unsigned int nPayloadSize = stParam.nCurValue;

        // 设置相机曝光组数为2
        int nRet = MV_CC_SetEnumValue(handle, "MultiLightControl", EXPOSURE_NUM);
        if (MV_OK != nRet)
        {
            printf("Warning:Set MultiLightControl fail，nRet:[%#x]\n", nRet);
        }

        // ch:设置触发模式为off | en:Set trigger mode as off
        nRet = MV_CC_SetEnumValue(handle, "TriggerMode", 0);
        if (MV_OK != nRet)
        {
            printf("Set Trigger Mode fail! nRet [0x%x]\n", nRet);
            break;
        }

        // ch:开始取流 | en:Start grab image
        nRet = MV_CC_StartGrabbing(handle);
        if (MV_OK != nRet)
        {
            printf("Start Grabbing fail! nRet [0x%x]\n", nRet);
            break;
        }

        MV_FRAME_OUT stImageInfo = {0};
        MV_RECONSTRUCT_IMAGE_PARAM stImgReconstructionParam = {0};
      
        unsigned int nReconstructBufferSize = 0;

        MV_CC_HB_DECODE_PARAM stDecodeParam = {0};
        

        nRet = MV_CC_GetImageBuffer(handle, &stImageInfo, 20000);//线阵相机需要更大的图像获取间隔
        if (nRet == MV_OK)
        {
            printf("Get Image Buffer: Width[%d], Height[%d], FrameNum[%d]\n",
                stImageInfo.stFrameInfo.nExtendWidth, stImageInfo.stFrameInfo.nExtendHeight, stImageInfo.stFrameInfo.nFrameNum);

            bool bpixel = IsHBPixelFormat(stImageInfo.stFrameInfo.enPixelType);
            if (bpixel)
            {
                stDecodeParam.pSrcBuf = stImageInfo.pBufAddr;
                stDecodeParam.nSrcLen = stImageInfo.stFrameInfo.nFrameLenEx;

                if (pDstBuf == NULL)
                {
                    pDstBuf = (unsigned char *)malloc(sizeof(unsigned char) * (nPayloadSize*3));
                    if (NULL == pDstBuf)
                    {
                        printf("malloc pDstData fail !\n");
                        nRet = MV_E_RESOURCE;
                        break;
                    }
                }
                stDecodeParam.pDstBuf = pDstBuf;
                stDecodeParam.nDstBufSize = nPayloadSize*3;
                nRet = MV_CC_HB_Decode(handle, &stDecodeParam);
                if (nRet != MV_OK)
                {
                    printf("Decode fail![0x%x]\n", nRet);
                    break;
                }
                else
                {
                    printf("HB Decode success!\n");
                }

                stImgReconstructionParam.nWidth =  stDecodeParam.nWidth;
                stImgReconstructionParam.nHeight = stDecodeParam.nHeight;
                stImgReconstructionParam.enPixelType = stDecodeParam.enDstPixelType;
                stImgReconstructionParam.pSrcData = stDecodeParam.pDstBuf;
                stImgReconstructionParam.nSrcDataLen = stDecodeParam.nDstBufLen;
                //申请总的拆分图像缓存，拆分后的图像保存在pReconstructBuffer中，后续可以进行存图处理
                nReconstructBufferSize = stDecodeParam.nDstBufLen;
                pReconstructBuffer = (unsigned char*)malloc(sizeof(unsigned char) * nReconstructBufferSize);
                if(NULL == pReconstructBuffer)
                {
                    printf("malloc pReconstructBuffer fail! nRet [0x%x]\n", nRet);
                    break;
                }

                //输出数据缓存赋值，频闪数为2
                // 自上而下进行拼图
                for (int i = 0; i < EXPOSURE_NUM; ++i)
                {
                    stImgReconstructionParam.stDstBufList[i].pBuf = pReconstructBuffer + i*(stDecodeParam.nDstBufLen / EXPOSURE_NUM);//pReconstructBuffer偏移
                    stImgReconstructionParam.stDstBufList[i].nBufSize = stDecodeParam.nDstBufLen / EXPOSURE_NUM;
                }
            }
            else
            {
                //图像重构结构体赋值
                stImgReconstructionParam.nWidth = stImageInfo.stFrameInfo.nExtendWidth;
                stImgReconstructionParam.nHeight = stImageInfo.stFrameInfo.nExtendHeight;
                stImgReconstructionParam.enPixelType = stImageInfo.stFrameInfo.enPixelType;
                stImgReconstructionParam.pSrcData = stImageInfo.pBufAddr;
                stImgReconstructionParam.nSrcDataLen = stImageInfo.stFrameInfo.nFrameLenEx;
                //申请总的拆分图像缓存，拆分后的图像保存在pReconstructBuffer中，后续可以进行存图处理
                nReconstructBufferSize = stImageInfo.stFrameInfo.nFrameLenEx;
                pReconstructBuffer = (unsigned char*)malloc(sizeof(unsigned char) * nReconstructBufferSize);
                if(NULL == pReconstructBuffer)
                {
                    printf("malloc pReconstructBuffer fail! nRet [0x%x]\n", nRet);
                    break;
                }

                //输出数据缓存赋值，频闪数为2
                // 自上而下进行拼图
                for (int i = 0; i < EXPOSURE_NUM; ++i)
                {
                    stImgReconstructionParam.stDstBufList[i].pBuf = pReconstructBuffer + i*(stImageInfo.stFrameInfo.nFrameLenEx / EXPOSURE_NUM);//pReconstructBuffer偏移
                    stImgReconstructionParam.stDstBufList[i].nBufSize = stImageInfo.stFrameInfo.nFrameLenEx / EXPOSURE_NUM;
                }
            }

            stImgReconstructionParam.nExposureNum = EXPOSURE_NUM;
            stImgReconstructionParam.enReconstructMethod = MV_SPLIT_BY_LINE;
           
            // Split Image And Reconstruct Image
            nRet = MV_CC_ReconstructImage(handle, &stImgReconstructionParam);
            if (MV_OK != nRet)
            {
                printf("Reconstruct Image fail! nRet [0x%x]\n", nRet);
                break;
            }

            //存原始数据JPG图片
            char input_file[100] = { 0 };
            sprintf(input_file, "InPut_w%d_h%d.jpg", stImageInfo.stFrameInfo.nExtendWidth, stImageInfo.stFrameInfo.nExtendHeight);
            MV_SAVE_IMG_TO_FILE_PARAM saveImage;
            memset(&saveImage, 0, sizeof(MV_SAVE_IMG_TO_FILE_PARAM));
            saveImage.enPixelType = stImageInfo.stFrameInfo.enPixelType;
            saveImage.pData = stImageInfo.pBufAddr;                //原始图像缓存
            saveImage.nDataLen = stImageInfo.stFrameInfo.nFrameLenEx;//原始图像大小
            saveImage.nWidth = stImageInfo.stFrameInfo.nExtendWidth;     //原始图像宽
            saveImage.nHeight = stImageInfo.stFrameInfo.nExtendHeight;   //原始图像高
            saveImage.enImageType = MV_Image_Jpeg;
            saveImage.nQuality = 99;

            memcpy(saveImage.pImagePath, input_file, strlen(input_file) + 1);
            saveImage.iMethodValue = 1;
            nRet = MV_CC_SaveImageToFile(handle, &saveImage);
            if (nRet != MV_OK)
            {
                printf("raw  image save to File fail nRet[%x]\n", nRet);
            }
            else
            {
                printf("raw image save to File success,save to  %s\n", input_file);
            }




            // 存拆分拼接后bmp图片
            char output_file[256] = { 0 };
            sprintf(output_file, "OutPut_w%d_h%d.bmp", stImgReconstructionParam.nWidth, stImgReconstructionParam.nHeight);
            memset(&saveImage, 0, sizeof(MV_SAVE_IMG_TO_FILE_PARAM));
            saveImage.enPixelType = stImgReconstructionParam.enPixelType;
            saveImage.pData = pReconstructBuffer;               //拆分图像缓存
            saveImage.nDataLen = nReconstructBufferSize;        //拆分图像大小
            saveImage.nWidth = stImgReconstructionParam.nWidth; //拆分图像宽
            saveImage.nHeight = stImgReconstructionParam.nHeight; //拆分图像高
            saveImage.enImageType = MV_Image_Bmp;
            saveImage.nQuality = 99;

            memcpy(saveImage.pImagePath, output_file, strlen(output_file) + 1);
            saveImage.iMethodValue = 1;
            nRet = MV_CC_SaveImageToFile(handle, &saveImage);
            if (nRet != MV_OK)
            {
                printf("After reconstruction SaveImage To File fail nRet[%x]\n", nRet);
            }
            else
            {
                printf("After reconstruction SaveImage To File success,  save to  %s\n", output_file);
            }


            nRet = MV_CC_FreeImageBuffer(handle, &stImageInfo);
            if(nRet != MV_OK)
            {
                printf("Free Image Buffer fail! nRet [0x%x]\n", nRet);
            }
            
            if(NULL != pReconstructBuffer)
            {
                free(pReconstructBuffer);
                pReconstructBuffer = NULL;
            }

            if (NULL !=pDstBuf)
            {
                free(pDstBuf);
                pDstBuf = NULL;
            }
        }
        else
        {
            printf("Get Image fail! nRet [0x%x]\n", nRet);
        }

        // ch:停止取流 | en:Stop grab image
        nRet = MV_CC_StopGrabbing(handle);
        if (MV_OK != nRet)
        {
            printf("Stop Grabbing fail! nRet [0x%x]\n", nRet);
            break;
        }

        // ch:关闭设备 | Close device
        nRet = MV_CC_CloseDevice(handle);
        if (MV_OK != nRet)
        {
            printf("ClosDevice fail! nRet [0x%x]\n", nRet);
            break;
        }

        // ch:销毁句柄 | Destroy handle
        nRet = MV_CC_DestroyHandle(handle);
        if (MV_OK != nRet)
        {
            printf("Destroy Handle fail! nRet [0x%x]\n", nRet);
            break;
        }
        handle = NULL;
    } while (0);

    if(MV_OK != nRet)
    {
        if(NULL != pDstBuf)
        {
            free (pDstBuf);
            pDstBuf = NULL;
        }

        if(NULL != pReconstructBuffer)
        {
            free (pReconstructBuffer);
            pReconstructBuffer = NULL;
        }
    }
 
    if (handle != NULL)
    {
        MV_CC_DestroyHandle(handle);
        handle = NULL;
    }
    

    // ch:反初始化SDK | en:Finalize SDK
    MV_CC_Finalize();

    PressEnterToExit();

    return 0;
}
