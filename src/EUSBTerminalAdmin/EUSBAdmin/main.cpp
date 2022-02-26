#include "mainwindow.h"

#include <QApplication>

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

// Mouse HID
#define EUSB_DEVICE_VENDOR_ID  0xF3, 0x04 // little endian
#define EUSB_DEVICE_PRODUCT_ID 0x35, 0x02 // little endian

#pragma pack(push, mir, 1)
typedef struct _MOUSE_INPUT_REPORT {
    UINT8 Buttons;
    UINT8 X;
    UINT8 Y;
    UINT8 Wheel;
} MOUSE_INPUT_REPORT, * PMOUSE_INPUT_REPORT;
#pragma pack(pop, mir)

DECLARE_CONST_UNICODE_STRING(g_ManufacturerStringEnUs, L"Lab");
DECLARE_CONST_UNICODE_STRING(g_ProductStringEnUs, L"UDE MOUSE");
const USHORT AMERICAN_ENGLISH = 0x409;
const UCHAR g_LanguageDescriptor[] = { 4,3,9,4 };


const UCHAR g_UsbDeviceDescriptor[18] =
{
    0x12,                            // Descriptor size
    USB_DEVICE_DESCRIPTOR_TYPE,      // Device descriptor type
    0x00, 0x02,                      // USB 2.0
    0x00,                            // Device class (interface-class defined)
    0x00,                            // Device subclass
    0x00,                            // Device protocol
    0x40,                            // Maxpacket size for EP0
    EUSB_DEVICE_VENDOR_ID,           // Vendor ID
    EUSB_DEVICE_PRODUCT_ID,          // Product ID
    0x58,                            // LSB of firmware revision
    0x24,                            // MSB of firmware revision
    0x01,                            // Manufacture string index
    0x02,                            // Product string index
    0x00,                            // Serial number string index
    0x01                             // Number of configurations
};


const UCHAR g_UsbConfigDescriptorSet[] =
{
    // Configuration Descriptor Type
    0x9,                               // Descriptor Size
    USB_CONFIGURATION_DESCRIPTOR_TYPE, // Configuration Descriptor Type
    0x22, 0x00,                        // Length of this descriptor and all sub descriptors
    0x01,                              // Number of interfaces
    0x01,                              // Configuration number
    0x00,                              // Configuration string index
    0xA0,                              // Config characteristics - Bus Powered, Remote Wakeup
    0x32,                              // Max power consumption of device (in 2mA unit) : 100 mA

        // Interface  descriptor
        0x9,                                      // Descriptor size
        USB_INTERFACE_DESCRIPTOR_TYPE,            // Interface Association Descriptor Type
        0,                                        // bInterfaceNumber
        0,                                        // bAlternateSetting
        1,                                        // bNumEndpoints
        0x03,                                     // bInterfaceClass (HID)
        0x01,                                     // bInterfaceSubClass (Boot Interface)
        0x02,                                     // bInterfaceProtocol (Mouse)
        0x00,                                     // iInterface

        // HID Descriptor
        0x09,       // Descriptor size
        0x21,       // bDescriptorType (HID)
        0x11, 0x01, // HID Class Spec Version
        0x00,       // bCountryCode
        0x01,       // bNumDescriptors
        0x22,       // bDescriptorType (Report)
        0x3E, 0x00, // wDescriptorLength

        // Interrupt IN endpoint descriptor
        0x07,                           // Descriptor size
        USB_ENDPOINT_DESCRIPTOR_TYPE,   // Descriptor type
        g_InterruptEndpointAddress,     // Endpoint address and description
        USB_ENDPOINT_TYPE_INTERRUPT,    // bmAttributes - interrupt
        0x04, 0x00,                     // Max packet size = 4 bytes
        0x0A                            // Servicing interval for interrupt (10 ms/1 frame)
};

const UCHAR g_HIDMouseUsbReportDescriptor[] =
{

    0x05, 0x01, // Usage Page (Generic Desktop)
    0x09, 0x02, // Usage(Mouse)
    0xA1, 0x01, // Collection(Application)
    0x09, 0x01, // Usage(Pointer)
    0xA1, 0x00, // Collection(Physical)
    0x05, 0x09, // Usage Page(Button)
    0x19, 0x01, // Usage Minimum(Button 1)
    0x29, 0x03, // Usage Maximum(Button 3)
    0x15, 0x00, // Logical Minimum(0)
    0x25, 0x01, // Logical Maximum(1)
    0x95, 0x03, // Report Count(3)
    0x75, 0x01, // Report Size(1)
    0x81, 0x02, // Input(Data, Var, Abs, NWrp, Lin, Pref, NNul, Bit)
    0x95, 0x05, // Report Count(5)
    0x75, 0x01, // Report Size(1)
    0x81, 0x03, // Input(Cnst, Var, Abs, NWrp, Lin, Pref, NNul, Bit)
    0x05, 0x01, // Usage Page(Generic Desktop)
    0x09, 0x30, // Usage(X)
    0x09, 0x31, // Usage(Y)
    0x15, 0x81, // Logical Minimum(-127)
    0x25, 0x7F, // Logical Maximum(127)
    0x75, 0x08, // Report Size(8)
    0x95, 0x02, // Report Count(2)
    0x81, 0x06, // Input(Data, Var, Rel, NWrp, Lin, Pref, NNul, Bit)
    0x09, 0x38, // Usage(Wheel)
    0x15, 0x81, // Logical Minimum(-127)
    0x25, 0x7F, // Logical Maximum(127)
    0x75, 0x08, // Report Size(8)
    0x95, 0x01, // Report Count(1)
    0x81, 0x06, // Input(Data, Var, Rel, NWrp, Lin, Pref, NNul, Bit)
    0xC0,       // End Collection
    0xC0        // End Collection
};

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


int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainWindow w;
    w.show();
    return a.exec();
}
