#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <pthread.h>
#include "MvCameraControl.h"

bool g_bExit = false;
unsigned int g_nPayloadSize = 0;

// 等待用户输入enter键来结束取流或结束程序
// wait for user to input enter to stop grabbing or end the sample program
void PressEnterToExit(void)
{
    int c;
    while ( (c = getchar()) != '\n' && c != EOF );
    fprintf( stderr, "\nPress enter to exit.\n");
    while( getchar() != '\n');
    g_bExit = true;
    sleep(1);
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
        printf("Device Model Name: %s\n", pstMVDevInfo->SpecialInfo.stGigEInfo.chModelName);
        printf("CurrentIp: %d.%d.%d.%d\n" , nIp1, nIp2, nIp3, nIp4);
        printf("UserDefinedName: %s\n\n" , pstMVDevInfo->SpecialInfo.stGigEInfo.chUserDefinedName);
    }
    else if (pstMVDevInfo->nTLayerType == MV_USB_DEVICE)
    {
        printf("Device Model Name: %s\n", pstMVDevInfo->SpecialInfo.stUsb3VInfo.chModelName);
        printf("UserDefinedName: %s\n\n", pstMVDevInfo->SpecialInfo.stUsb3VInfo.chUserDefinedName);
    }
    else
    {
        printf("Not support.\n");
    }

    return true;
}

static void* WorkThread(void* pUser)
{
    int nRet = MV_OK;

	MV_FRAME_OUT stImageInfo = {0};
    memset(&stImageInfo, 0, sizeof(MV_FRAME_OUT));
    

    while(1)
    {
		if(g_bExit)
		{
			break;
		}
			
        nRet = MV_CC_GetImageBuffer(pUser, &stImageInfo, 1000);
        if (nRet == MV_OK)
        {
			//Print parse the timestamp information in the frame
            printf("Get One Frame: Chunk_ExposureTime[%f], Chunk_SecondCount[%d], Chunk_CycleCount[%d], Chunk_CycleOffset[%d], nFrameNum[%d]\n", 
                stImageInfo.stFrameInfo.fExposureTime, stImageInfo.stFrameInfo.nSecondCount, stImageInfo.stFrameInfo.nCycleCount, 
				stImageInfo.stFrameInfo.nCycleOffset, stImageInfo.stFrameInfo.nFrameNum);
			MV_CHUNK_DATA_CONTENT* pUnparsedChunkContent = stImageInfo.stFrameInfo.UnparsedChunkList.pUnparsedChunkContent;
			for(unsigned int i = 0;i < stImageInfo.stFrameInfo.nUnparsedChunkNum; i++)
			{
				//ch：只打印ID和长度，内容需要根据说明文档进行解析  | en: print id and length, the content needs to be parsed according to the document
				printf("ChunkInfo[%d]: ChunkID[0x%x], ChunkLen[%d]\n", i, pUnparsedChunkContent->nChunkID, pUnparsedChunkContent->nChunkLen);
				pUnparsedChunkContent++;
			}
			printf("***********************************\n");
			
			MV_CC_FreeImageBuffer(pUser, &stImageInfo);
        }
        else{
            printf("No data[%x]\n", nRet);
        }
    }

    return 0;
}

int main()
{
    int nRet = MV_OK;

    void* handle = NULL;

    do 
    {
        // ch:初始化SDK | en:Initialize SDK
		nRet = MV_CC_Initialize();
		if (MV_OK != nRet)
		{
			printf("Initialize SDK fail! nRet [0x%x]\n", nRet);
			break;
		}

        MV_CC_DEVICE_INFO_LIST stDeviceList;
        memset(&stDeviceList, 0, sizeof(MV_CC_DEVICE_INFO_LIST));

        // 枚举设备
        // enum device
        nRet = MV_CC_EnumDevices(MV_GIGE_DEVICE | MV_USB_DEVICE, &stDeviceList);
        if (MV_OK != nRet)
        {
            printf("MV_CC_EnumDevices fail! nRet [%x]\n", nRet);
            break;
        }

        if (stDeviceList.nDeviceNum > 0)
        {
            for (int i = 0; i < stDeviceList.nDeviceNum; i++)
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

        printf("Please Intput camera index:");
        unsigned int nIndex = 0;
        scanf("%d", &nIndex);

        if (nIndex >= stDeviceList.nDeviceNum)
        {
            printf("Intput error!\n");
            break;
        }

        // 选择设备并创建句柄
        // select device and create handle
        nRet = MV_CC_CreateHandle(&handle, stDeviceList.pDeviceInfo[nIndex]);
        if (MV_OK != nRet)
        {
            printf("MV_CC_CreateHandle fail! nRet [%x]\n", nRet);
            break;
        }

        // 打开设备
        // open device
        nRet = MV_CC_OpenDevice(handle);
        if (MV_OK != nRet)
        {
            printf("MV_CC_OpenDevice fail! nRet [%x]\n", nRet);
            break;
        }
		
		// ch:开启Chunk Mode | en:Open Chunk Mode
        nRet = MV_CC_SetBoolValue(handle, "ChunkModeActive", true);
        if (MV_OK != nRet)
        {
            printf("Set Chunk Mode fail! nRet [0x%x]\n", nRet);
            break;
        }

        // ch:Chunk Selector设为Exposure | en: Chunk Selector set as Exposure
        nRet = MV_CC_SetEnumValueByString(handle, "ChunkSelector", "Exposure");
        if (MV_OK != nRet)
        {
            printf("Set Exposure Chunk fail! nRet [0x%x]\n", nRet);
            break;
        }

        // ch:开启Chunk Enable | en:Open Chunk Enable
        nRet = MV_CC_SetBoolValue(handle, "ChunkEnable", true);
        if (MV_OK != nRet)
        {
            printf("Set Chunk Enable fail! nRet [0x%x]\n", nRet);
            break;
        }

        // ch:Chunk Selector设为Timestamp | en: Chunk Selector set as Timestamp
        nRet = MV_CC_SetEnumValueByString(handle, "ChunkSelector", "Timestamp");
        if (MV_OK != nRet)
        {
            printf("Set Timestamp Chunk fail! nRet [0x%x]\n", nRet);
            break;
        }

        // ch:开启Chunk Enable | en:Open Chunk Enable
        nRet = MV_CC_SetBoolValue(handle, "ChunkEnable", true);
        if (MV_OK != nRet)
        {
            printf("Set Chunk Enable fail! nRet [0x%x]\n", nRet);
            break;
        }

        // ch:探测网络最佳包大小(只对GigE相机有效) | en:Detection network optimal package size(It only works for the GigE camera)
        if (stDeviceList.pDeviceInfo[nIndex]->nTLayerType == MV_GIGE_DEVICE)
        {
            int nPacketSize = MV_CC_GetOptimalPacketSize(handle);
            if (nPacketSize > 0)
            {
                nRet = MV_CC_SetIntValueEx(handle,"GevSCPSPacketSize",nPacketSize);
                if(nRet != MV_OK)
                {
                    printf("Warning: Set Packet Size fail nRet [0x%x]!\n", nRet);
                }
            }
            else
            {
                printf("Warning: Get Packet Size fail nRet [0x%x]!\n", nPacketSize);
            }
        }
		
        // 设置触发模式为off
        // set trigger mode as off
        nRet = MV_CC_SetEnumValue(handle, "TriggerMode", 0);
        if (MV_OK != nRet)
        {
            printf("MV_CC_SetTriggerMode fail! nRet [%x]\n", nRet);
            break;
        }

        // 开始取流
        // start grab image
        nRet = MV_CC_StartGrabbing(handle);
        if (MV_OK != nRet)
        {
            printf("MV_CC_StartGrabbing fail! nRet [%x]\n", nRet);
            break;
        }

        pthread_t nThreadID;
        nRet = pthread_create(&nThreadID, NULL ,WorkThread , handle);
        if (nRet != 0)
        {
            printf("thread create failed.ret = %d\n",nRet);
            break;
        }

        PressEnterToExit();

        // 停止取流
        // end grab image
        nRet = MV_CC_StopGrabbing(handle);
        if (MV_OK != nRet)
        {
            printf("MV_CC_StopGrabbing fail! nRet [%x]\n", nRet);
            break;
        }

        // 关闭设备
        // close device
        nRet = MV_CC_CloseDevice(handle);
        if (MV_OK != nRet)
        {
            printf("MV_CC_CloseDevice fail! nRet [%x]\n", nRet);
            break;
        }

        // 销毁句柄
        // destroy handle
        nRet = MV_CC_DestroyHandle(handle);
        if (MV_OK != nRet)
        {
            printf("MV_CC_DestroyHandle fail! nRet [%x]\n", nRet);
            break;
        }
        handle = NULL;
    } while (0);


    if (handle != NULL)
    {
        MV_CC_DestroyHandle(handle);
        handle = NULL;
    }
    

    // ch:反初始化SDK | en:Finalize SDK
	MV_CC_Finalize();

    printf("exit.\n");
    return 0;
}
