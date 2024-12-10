/*
* ���ʾ����ʾ���������֡�������д�����ؽڵ�IO�����á�
* This sample shows how to modify FrameBurstStart and LineStart mode of the linear camera.
*/


#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include "MvCameraControl.h"

bool g_bExit = false;

// ch:�ȴ��������� | en:Wait for key press
void PressEnterToExit(void)
{
    int c;
    while ( (c = getchar()) != '\n' && c != EOF );
    fprintf( stderr, "\nPress enter to exit.\n");
    while( getchar() != '\n');
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

        // ch:��ӡ��ǰ���ip���û��Զ������� | en:print current ip and user defined name
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

void __stdcall ImageCallBackEx(unsigned char * pData, MV_FRAME_OUT_INFO_EX* pFrameInfo, void* pUser)
{
	if (pFrameInfo)
	{
		printf("Get One Frame: Width[%d], Height[%d], nFrameNum[%d]\n", 
			pFrameInfo->nExtendWidth, pFrameInfo->nExtendHeight, pFrameInfo->nFrameNum);
	}
}

int main()
{
    int nRet = MV_OK;
    void* handle = NULL;

    do 
    {
		// ch:��ʼ��SDK | en:Initialize SDK
		nRet = MV_CC_Initialize();
		if (MV_OK != nRet)
		{
			printf("Initialize SDK fail! nRet [0x%x]\n", nRet);
			break;
		}

        // ch:ö���豸 | en:Enum device
        MV_CC_DEVICE_INFO_LIST stDeviceList;
        memset(&stDeviceList, 0, sizeof(MV_CC_DEVICE_INFO_LIST));
        nRet = MV_CC_EnumDevices(MV_GIGE_DEVICE | MV_USB_DEVICE | MV_GENTL_CAMERALINK_DEVICE | MV_GENTL_CXP_DEVICE | MV_GENTL_XOF_DEVICE, &stDeviceList);
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

        // ch:ѡ���豸��������� | en:Select device and create handle
        nRet = MV_CC_CreateHandle(&handle, stDeviceList.pDeviceInfo[nIndex]);
        if (MV_OK != nRet)
        {
            printf("Create Handle fail! nRet [0x%x]\n", nRet);
            break;
        }

        // ch:���豸 | en:Open device
        nRet = MV_CC_OpenDevice(handle);
        if (MV_OK != nRet)
        {
            printf("Open Device fail! nRet [0x%x]\n", nRet);
            break;
        }

        // ch:̽��������Ѱ���С(ֻ��GigE�����Ч) | en:Detection network optimal package size(It only works for the GigE camera)
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

		printf("Please Input trigger selector index(0-1): 0-FrameBurstStart, 1-LineStart\n");
		unsigned int nTriggerSelector = 0;
		scanf("%d", &nTriggerSelector);

		if (nTriggerSelector == 0)
		{
			// ch:���ô���ѡ��ΪFrameBurstStart | en:Set trigger selector as FrameBurstStart
			nRet = MV_CC_SetEnumValue(handle, "TriggerSelector", 6);
			if (MV_OK != nRet)
			{
				printf("Set Trigger Selector fail! nRet [0x%x]\n", nRet);
				break;
			}

			// ch:���ô���ģʽΪon | en:Set trigger mode as on
			nRet = MV_CC_SetEnumValue(handle, "TriggerMode", 1);
			if (MV_OK != nRet)
			{
				printf("Set Trigger Mode fail! nRet [0x%x]\n", nRet);
				break;
			}

			// ch:���ô���ԴΪLine0 | en:Set trigger source as Line0
			nRet = MV_CC_SetEnumValue(handle, "TriggerSource", 0);
			if (MV_OK != nRet)
			{
				printf("Set Trigger source fail! nRet [0x%x]\n", nRet);
				break;
			}
		}
		else if (nTriggerSelector == 1)
		{
			// ch:���ô���ѡ��ΪLineStart | en:Set trigger selector as LineStart
			nRet = MV_CC_SetEnumValue(handle, "TriggerSelector", 9);
			if (MV_OK != nRet)
			{
				printf("Set Trigger Selector fail! nRet [0x%x]\n", nRet);
				break;
			}

			// ch:���ô���ģʽΪon | en:Set trigger mode as on
			nRet = MV_CC_SetEnumValue(handle, "TriggerMode", 1);
			if (MV_OK != nRet)
			{
				printf("Set Trigger Mode fail! nRet [0x%x]\n", nRet);
				break;
			}

			// ch:���ô���ԴΪEncoderModuleOut | en:Set trigger source as EncoderModuleOut
			nRet = MV_CC_SetEnumValue(handle, "TriggerSource", 6);
			if (MV_OK != nRet)
			{
				printf("Set Trigger source fail! nRet [0x%x]\n", nRet);
				break;
			}

			// ch:���ñ�����ѡ��ΪEncoder0 | en:Set encoder selector as Encoder0
			nRet = MV_CC_SetEnumValue(handle, "EncoderSelector", 0);
			if (MV_OK != nRet)
			{
				printf("Set encoder selector fail! nRet [0x%x]\n", nRet);
				break;
			}

			// ch:���ñ���������ԴAΪLine1 | en:Set encoder source A as Line1
			nRet = MV_CC_SetEnumValue(handle, "EncoderSourceA", 1);
			if (MV_OK != nRet)
			{
				printf("Set encoder sourceA fail! nRet [0x%x]\n", nRet);
				break;
			}

			// ch:���ñ���������ԴBΪLine3 | en:Set encoder source B as Line3
			nRet = MV_CC_SetEnumValue(handle, "EncoderSourceB", 3);
			if (MV_OK != nRet)
			{
				printf("Set encoder sourceB fail! nRet [0x%x]\n", nRet);
				break;
			}
		}
		else
		{
			 printf("Input error!\n");
			break;
		}

		// ch:ע��ץͼ�ص� | en:Register image callback
		nRet = MV_CC_RegisterImageCallBackEx(handle, ImageCallBackEx, handle);
		if (MV_OK != nRet)
		{
			printf("Register Image CallBack fail! nRet [0x%x]\n", nRet);
			break;
		}

        // ch:��ʼȡ�� | en:Start grab image
        nRet = MV_CC_StartGrabbing(handle);
        if (MV_OK != nRet)
        {
            printf("Start Grabbing fail! nRet [0x%x]\n", nRet);
            break;
        }

        PressEnterToExit();
        // ch:ֹͣȡ�� | en:Stop grab image
        nRet = MV_CC_StopGrabbing(handle);
        if (MV_OK != nRet)
        {
            printf("Stop Grabbing fail! nRet [0x%x]\n", nRet);
            break;
        }

		// ch:ע��ץͼ�ص� | en:Unregister image callback
		nRet = MV_CC_RegisterImageCallBackEx(handle, NULL, NULL);
		if (MV_OK != nRet)
		{
			printf("Unregister Image CallBack fail! nRet [0x%x]\n", nRet);
			break;
		}

        // ch:�ر��豸 | Close device
        nRet = MV_CC_CloseDevice(handle);
        if (MV_OK != nRet)
        {
            printf("ClosDevice fail! nRet [0x%x]\n", nRet);
            break;
        }

        // ch:���پ�� | Destroy handle
        nRet = MV_CC_DestroyHandle(handle);
        if (MV_OK != nRet)
        {
            printf("Destroy Handle fail! nRet [0x%x]\n", nRet);
            break;
        }
		handle = NULL;
    } while (0);
 
	if (handle != NULL)
	{
		MV_CC_DestroyHandle(handle);
		handle = NULL;
	}
    

	// ch:����ʼ��SDK | en:Finalize SDK
	MV_CC_Finalize();

    return 0;
}