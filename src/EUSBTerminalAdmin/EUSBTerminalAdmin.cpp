#define _CRT_SECURE_NO_WARNINGS

#include <DriverSpecs.h>

#include <iostream>
#include <Windows.h>
#include <cfgmgr32.h>
#include <basetyps.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <conio.h>
#include "devioctl.h"
#include "strsafe.h"



#define EUSB_BUFF_TAG 'EUSB'
#define MAX_DEVPATH_LENGTH 256

//
// Define an Interface Guid so that apps can find the device and talk to it.
//
#include <initguid.h>
DEFINE_GUID(GUID_DEVINTERFACE_EUSB,
    0x752808f4, 0xa9e9, 0x42f4, 0x81, 0x15, 0x64, 0x40, 0xc9, 0x87, 0x13, 0xb1);
// {752808f4-a9e9-42f4-8115-6440c98713b1}

BOOL
GetDevicePath(
    _In_  LPGUID InterfaceGuid,
    _Out_writes_z_(BufLen) PWCHAR DevicePath,
    _In_ size_t BufLen
)
{
    CONFIGRET cr = CR_SUCCESS;
    PWSTR deviceInterfaceList = NULL;
    ULONG deviceInterfaceListLength = 0;
    PWSTR nextInterface;
    HRESULT hr = E_FAIL;
    BOOL bRet = TRUE;

    cr = CM_Get_Device_Interface_List_Size(
        &deviceInterfaceListLength,
        InterfaceGuid,
        NULL,
        CM_GET_DEVICE_INTERFACE_LIST_PRESENT);
    if (cr != CR_SUCCESS) {
        printf("Error 0x%x retrieving device interface list size.\n", cr);
        goto clean0;
    }

    if (deviceInterfaceListLength <= 1) {
        bRet = FALSE;
        printf("Error: No active device interfaces found.\n"
            " Is the sample driver loaded?");
        goto clean0;
    }

    deviceInterfaceList = (PWSTR)malloc(deviceInterfaceListLength * sizeof(WCHAR));
    if (deviceInterfaceList == NULL) {
        bRet = FALSE;
        printf("Error allocating memory for device interface list.\n");
        goto clean0;
    }
    ZeroMemory(deviceInterfaceList, deviceInterfaceListLength * sizeof(WCHAR));

    cr = CM_Get_Device_Interface_List(
        InterfaceGuid,
        NULL,
        deviceInterfaceList,
        deviceInterfaceListLength,
        CM_GET_DEVICE_INTERFACE_LIST_PRESENT);
    if (cr != CR_SUCCESS) {
        printf("Error 0x%x retrieving device interface list.\n", cr);
        goto clean0;
    }

    nextInterface = deviceInterfaceList + wcslen(deviceInterfaceList) + 1;
    if (*nextInterface != UNICODE_NULL) {
        printf("Warning: More than one device interface instance found. \n"
            "Selecting first matching device.\n\n");
    }

    hr = StringCchCopy(DevicePath, BufLen, deviceInterfaceList);
    if (FAILED(hr)) {
        bRet = FALSE;
        printf("Error: StringCchCopy failed with HRESULT 0x%x", hr);
        goto clean0;
    }

clean0:
    if (deviceInterfaceList != NULL) {
        free(deviceInterfaceList);
    }
    if (CR_SUCCESS != cr) {
        bRet = FALSE;
    }

    return bRet;
}


HANDLE OpenDeviceByGuid(_In_ LPCGUID pguid)
{
    HANDLE hDev;
    WCHAR completeDeviceName[MAX_DEVPATH_LENGTH];

    if (!GetDevicePath(
        (LPGUID)pguid,
        completeDeviceName,
        sizeof(completeDeviceName) / sizeof(completeDeviceName[0])))
    {
        return  INVALID_HANDLE_VALUE;
    }

    printf("DeviceName = (%S)\n", completeDeviceName); fflush(stdout);

    hDev = CreateFile(completeDeviceName,
        GENERIC_WRITE | GENERIC_READ,
        FILE_SHARE_WRITE | FILE_SHARE_READ,
        NULL, // default security
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL);

    if (hDev == INVALID_HANDLE_VALUE) {
        printf("Failed to open the device, error - %d", GetLastError()); fflush(stdout);
    }
    else {
        printf("Opened the device successfully.\n"); fflush(stdout);
    }

    return hDev;
}


int main()
{
    std::cout << "EUSB Terminal Admin\n";
    std::cout << "Thread id: " << GetCurrentThreadId() << std::endl;
    HANDLE hcDeviceHandle;
    hcDeviceHandle = OpenDeviceByGuid((LPGUID)&GUID_DEVINTERFACE_EUSB);

    if (hcDeviceHandle == INVALID_HANDLE_VALUE) {

        printf("Unable to find device!\n"); 
        fflush(stdout);
        return 1;
    }

    std::cout << "Host controller device successfully found" << std::endl;
    int _index = 0;

    char cmd = ' ';
    while (cmd != 'q')
    {
        cmd = _getch();
        switch (cmd) 
        {
        case 'h':
            std::cout << "q - quit\n"
                << "i - IO device control\n"
                << "w - write number\n";
            break;
        case 'q':
            goto exit;
            break;
        case 'i':
            DeviceIoControl(hcDeviceHandle, 1000, NULL, NULL, NULL, NULL, NULL, NULL);
            break;
        case 'w':
            int wdnum;
            char buff[32];
            sprintf(buff, "index-%d", _index);
            WriteFile(hcDeviceHandle, buff, strlen(buff), (LPDWORD)&wdnum, NULL);
            _index++;   
            std::cout << "Writed " << wdnum << "bytes..." << std::endl;
            break;
        }
    }
    

exit:
    CloseHandle(hcDeviceHandle);
    return 0;
}
