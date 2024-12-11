# -- coding: utf-8 --

import sys
import os
import configparser

sys.path.append(os.path.join(os.path.dirname(__file__), 'libs'))

from datetime import datetime
from ctypes import *
from MvCameraControl_class import *

Config = configparser.ConfigParser()
Config.read(os.path.join(os.path.dirname(__file__), 'config.ini'))

current_time = datetime.now()

env = Config.get("Env","Mode")

HB_format_list = [
    PixelType_Gvsp_HB_Mono8,
    PixelType_Gvsp_HB_Mono10,
    PixelType_Gvsp_HB_Mono10_Packed,
    PixelType_Gvsp_HB_Mono12,
    PixelType_Gvsp_HB_Mono12_Packed,
    PixelType_Gvsp_HB_Mono16,
    PixelType_Gvsp_HB_BayerGR8,
    PixelType_Gvsp_HB_BayerRG8,
    PixelType_Gvsp_HB_BayerGB8,
    PixelType_Gvsp_HB_BayerBG8,
    PixelType_Gvsp_HB_BayerRBGG8,
    PixelType_Gvsp_HB_BayerGR10,
    PixelType_Gvsp_HB_BayerRG10,
    PixelType_Gvsp_HB_BayerGB10,
    PixelType_Gvsp_HB_BayerBG10,
    PixelType_Gvsp_HB_BayerGR12,
    PixelType_Gvsp_HB_BayerRG12,
    PixelType_Gvsp_HB_BayerGB12,
    PixelType_Gvsp_HB_BayerBG12,
    PixelType_Gvsp_HB_BayerGR10_Packed,
    PixelType_Gvsp_HB_BayerRG10_Packed,
    PixelType_Gvsp_HB_BayerGB10_Packed,
    PixelType_Gvsp_HB_BayerBG10_Packed,
    PixelType_Gvsp_HB_BayerGR12_Packed,
    PixelType_Gvsp_HB_BayerRG12_Packed,
    PixelType_Gvsp_HB_BayerGB12_Packed,
    PixelType_Gvsp_HB_BayerBG12_Packed,
    PixelType_Gvsp_HB_YUV422_Packed,
    PixelType_Gvsp_HB_YUV422_YUYV_Packed,
    PixelType_Gvsp_HB_RGB8_Packed,
    PixelType_Gvsp_HB_BGR8_Packed,
    PixelType_Gvsp_HB_RGBA8_Packed,
    PixelType_Gvsp_HB_BGRA8_Packed,
    PixelType_Gvsp_HB_RGB16_Packed,
    PixelType_Gvsp_HB_BGR16_Packed,
    PixelType_Gvsp_HB_RGBA16_Packed,
    PixelType_Gvsp_HB_BGRA16_Packed]


class Image:

    def __save_non_raw_image(self, save_type, frame_info, cam_instance):
        folder_name = current_time.strftime("%d-%m-%Y")
        file_name = current_time.strftime("%H-%M-%S-%f")

        if save_type == 1:
            mv_image_type = MV_Image_Jpeg
            file_extension = "jpg"
        elif save_type == 2:
            mv_image_type = MV_Image_Bmp
            file_extension = "bmp"
        elif save_type == 3:
            mv_image_type = MV_Image_Tif
            file_extension = "tif"
        else:
            mv_image_type = MV_Image_Png
            file_extension = "png"

        folder_path = os.path.join(os.path.dirname(Config.get("Image","Save_path")), folder_name)
        if not os.path.exists(folder_path):
            os.makedirs(folder_path)

        file_path = os.path.join(folder_path, f"{file_name}_w{frame_info.stFrameInfo.nWidth}_h{frame_info.stFrameInfo.nHeight}_fn{frame_info.stFrameInfo.nFrameNum}.{file_extension}")

        c_file_path = file_path.encode('ascii')
        stSaveParam = MV_SAVE_IMAGE_TO_FILE_PARAM_EX()
        stSaveParam.enPixelType = frame_info.stFrameInfo.enPixelType
        stSaveParam.nWidth = frame_info.stFrameInfo.nWidth
        stSaveParam.nHeight = frame_info.stFrameInfo.nHeight
        stSaveParam.nDataLen = frame_info.stFrameInfo.nFrameLen
        stSaveParam.pData = cast(frame_info.pBufAddr, POINTER(c_ubyte))
        stSaveParam.enImageType = mv_image_type
        stSaveParam.nQuality = 80
        stSaveParam.pcImagePath = create_string_buffer(c_file_path)
        stSaveParam.iMethodValue = 0

        mv_ret = cam_instance.MV_CC_SaveImageToFileEx(stSaveParam)
        return mv_ret
    
    def __save_raw(self, frame_info, cam_instance):
        current_time = datetime.now()
        folder_name = current_time.strftime("%d-%m-%Y")
        file_name = current_time.strftime("%H-%M-%S-%f")

        folder_path = os.path.join(os.path.dirname(Config.get("Image", "Save_path")), folder_name)
        if not os.path.exists(folder_path):
            os.makedirs(folder_path)

        if frame_info.stFrameInfo.enPixelType in HB_format_list:
            stDecodeParam = MV_CC_HB_DECODE_PARAM()
            memset(byref(stDecodeParam), 0, sizeof(stDecodeParam))

            stParam = MVCC_INTVALUE()
            memset(byref(stParam), 0, sizeof(stParam))

            ret = cam_instance.MV_CC_GetIntValue("PayloadSize", stParam)
            if 0 != ret:
                print("Get PayloadSize fail! ret[0x%x]" % ret)
                return ret
            nPayloadSize = stParam.nCurValue
            stDecodeParam.pSrcBuf = frame_info.pBufAddr
            stDecodeParam.nSrcLen = frame_info.stFrameInfo.nFrameLen
            stDecodeParam.pDstBuf = (c_ubyte * nPayloadSize)()
            stDecodeParam.nDstBufSize = nPayloadSize
            ret = cam_instance.MV_CC_HBDecode(stDecodeParam)
            if ret != 0:
                print("HB Decode fail! ret[0x%x]" % ret)
                return ret
            else:
                file_path = os.path.join(folder_path, f"{file_name}_w{stDecodeParam.nWidth}_h{stDecodeParam.nHeight}_fn{frame_info.stFrameInfo.nFrameNum}.raw")
                try:
                    with open(file_path, 'wb+') as file_open:
                        img_save = (c_ubyte * stDecodeParam.nDstBufLen)()
                        memmove(byref(img_save), stDecodeParam.pDstBuf, stDecodeParam.nDstBufLen)
                        file_open.write(img_save)
                except PermissionError:
                    print("save error raw file executed failed!")
                    return MV_E_OPENFILE
        else:
            file_path = os.path.join(folder_path, f"{file_name}_w{frame_info.stFrameInfo.nWidth}_h{frame_info.stFrameInfo.nHeight}_fn{frame_info.stFrameInfo.nFrameNum}.raw")
            try:
                with open(file_path, 'wb+') as file_open:
                    img_save = (c_ubyte * frame_info.stFrameInfo.nFrameLen)()
                    memmove(byref(img_save), frame_info.pBufAddr, frame_info.stFrameInfo.nFrameLen)
                    file_open.write(img_save)
            except PermissionError:
                print("save error raw file executed failed!")
                return MV_E_OPENFILE
        return 0
    
    def save(self):
        try:
            MvCamera.MV_CC_Initialize()

            deviceList = MV_CC_DEVICE_INFO_LIST()
            tlayerType = (MV_GIGE_DEVICE | MV_USB_DEVICE | MV_GENTL_CAMERALINK_DEVICE
                        | MV_GENTL_CXP_DEVICE | MV_GENTL_XOF_DEVICE)
            selectedDeviceIndex = None

            ret = MvCamera.MV_CC_EnumDevices(tlayerType, deviceList)
            if ret != 0:
                print("enum devices fail! ret[0x%x]" % ret)
                sys.exit()

            if deviceList.nDeviceNum == 0:
                print("find no device!")
                sys.exit()

            if env == "dev" : print("Find %d devices!" % deviceList.nDeviceNum)

            for i in range(0, deviceList.nDeviceNum):
                mvcc_dev_info = cast(deviceList.pDeviceInfo[i], POINTER(MV_CC_DEVICE_INFO)).contents

                if mvcc_dev_info.nTLayerType == MV_GIGE_DEVICE or mvcc_dev_info.nTLayerType == MV_GENTL_GIGE_DEVICE:

                    # Get device name
                    strModeName = ""
                    for per in mvcc_dev_info.SpecialInfo.stGigEInfo.chModelName:
                        if per == 0:
                            break
                        strModeName = strModeName + chr(per)

                    # Get IP address
                    nip1 = ((mvcc_dev_info.SpecialInfo.stGigEInfo.nCurrentIp & 0xff000000) >> 24)
                    nip2 = ((mvcc_dev_info.SpecialInfo.stGigEInfo.nCurrentIp & 0x00ff0000) >> 16)
                    nip3 = ((mvcc_dev_info.SpecialInfo.stGigEInfo.nCurrentIp & 0x0000ff00) >> 8)
                    nip4 = (mvcc_dev_info.SpecialInfo.stGigEInfo.nCurrentIp & 0x000000ff)

                    ipAddress = "%d.%d.%d.%d" % (nip1, nip2, nip3, nip4)

                    if strModeName == Config.get("Camera","Name") and ipAddress == Config.get("Camera","IP"):
                        if env == "dev" : print("Conected to the camera!, IP : %s Name : %s" % (ipAddress, strModeName))
                        selectedDeviceIndex = i

            nSaveImageType = Config.get("Image","Type").lower()

            if env == "dev" : print("Save image type : %s" % nSaveImageType)

            if nSaveImageType == "raw":    nSaveImageType = 0
            elif nSaveImageType == "jpeg": nSaveImageType = 1
            elif nSaveImageType == "bmp":  nSaveImageType = 2
            elif nSaveImageType == "tiff": nSaveImageType = 3
            elif nSaveImageType == "png":  nSaveImageType = 4
            else:
                print("Image type error!")
                sys.exit()

            cam = MvCamera()

            stDeviceList = cast(deviceList.pDeviceInfo[int(selectedDeviceIndex)], POINTER(MV_CC_DEVICE_INFO)).contents

            ret = cam.MV_CC_CreateHandle(stDeviceList)
            if ret != 0:
                raise Exception("create handle fail! ret[0x%x]" % ret)

            ret = cam.MV_CC_OpenDevice(MV_ACCESS_Exclusive, 0)
            if ret != 0:
                raise Exception("open device fail! ret[0x%x]" % ret)

            if stDeviceList.nTLayerType == MV_GIGE_DEVICE or stDeviceList.nTLayerType == MV_GENTL_GIGE_DEVICE:
                nPacketSize = cam.MV_CC_GetOptimalPacketSize()
                if int(nPacketSize) > 0:
                    ret = cam.MV_CC_SetIntValue("GevSCPSPacketSize", nPacketSize)
                    if ret != 0:
                        print("Warning: Set Packet Size fail! ret[0x%x]" % ret)
                else:
                    print("Warning: Get Packet Size fail! ret[0x%x]" % nPacketSize)

            ret = cam.MV_CC_SetEnumValue("TriggerMode", MV_TRIGGER_MODE_OFF)
            if ret != 0:
                raise Exception("set trigger mode fail! ret[0x%x]" % ret)

            ret = cam.MV_CC_StartGrabbing()
            if ret != 0:
                raise Exception("start grabbing fail! ret[0x%x]" % ret)

            stOutFrame = MV_FRAME_OUT()
            
            memset(byref(stOutFrame), 0, sizeof(stOutFrame))

            ret = cam.MV_CC_GetImageBuffer(stOutFrame, 20000)
            if None != stOutFrame.pBufAddr and 0 == ret:
                if env == "dev" : print("get one frame: Width[%d], Height[%d], nFrameNum[%d]" % (
                stOutFrame.stFrameInfo.nWidth, stOutFrame.stFrameInfo.nHeight, stOutFrame.stFrameInfo.nFrameNum))

                if int(nSaveImageType) == 0:
                    ret = self.__save_raw(stOutFrame, cam)
                else:
                    ret = self.__save_non_raw_image(int(nSaveImageType), stOutFrame, cam)
                if ret != 0:
                    cam.MV_CC_FreeImageBuffer(stOutFrame)
                    raise Exception("save image fail! ret[0x%x]" % ret)
                else:
                    atTime = current_time.strftime("%H-%M-%S-%f")
                    print("Save image success! at %s" % atTime)

                cam.MV_CC_FreeImageBuffer(stOutFrame)
            else:
                raise Exception("no data[0x%x]" % ret)

            ret = cam.MV_CC_StopGrabbing()
            if ret != 0:
                raise Exception("stop grabbing fail! ret[0x%x]" % ret)

            ret = cam.MV_CC_CloseDevice()
            if ret != 0:
                raise Exception("close device fail! ret[0x%x]" % ret)

            cam.MV_CC_DestroyHandle()

        except Exception as e:
            print(e)
            cam.MV_CC_CloseDevice()
            cam.MV_CC_DestroyHandle()
        finally:
            MvCamera.MV_CC_Finalize()

    
