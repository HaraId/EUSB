#pragma once

#include "Device.h"
#include "Trace.h"

typedef struct _USB_CONTEXT {
    WDFDEVICE             ControllerDevice;
    UDECXUSBENDPOINT      UDEFX2ControlEndpoint;
    UDECXUSBENDPOINT      UDEFX2BulkOutEndpoint;
    UDECXUSBENDPOINT      UDEFX2BulkInEndpoint;
    UDECXUSBENDPOINT      UDEFX2InterruptInEndpoint;


    WDFQUEUE          ControlQueue;
    WDFQUEUE          InterruptUrbQueue;
    WDFQUEUE          IntrDeferredQueue;
    
} USB_CONTEXT, *PUSB_CONTEXT;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(USB_CONTEXT, GetUsbDeviceContext);

EXTERN_C_START


// ----- descriptor constants/strings/indexes
#define g_ManufacturerIndex   1
#define g_ProductIndex        2
#define g_BulkOutEndpointAddress 2
#define g_BulkInEndpointAddress    0x84
#define g_InterruptEndpointAddress 0x86

// Mouse HID 
#define EUSB_DEVICE_VENDOR_ID  0xF3, 0x04 // little endian
#define EUSB_DEVICE_PRODUCT_ID 0x35, 0x02 // little endian

extern const UCHAR g_UsbDeviceDescriptor[];
extern const UCHAR g_UsbConfigDescriptorSet[];
extern const UCHAR g_HIDMouseUsbReportDescriptor[];


#pragma pack(push, mir, 1)
typedef struct _MOUSE_INPUT_REPORT {
    UINT8 Buttons;
    UINT8 X;
    UINT8 Y;
    UINT8 Wheel;
} MOUSE_INPUT_REPORT, * PMOUSE_INPUT_REPORT;
#pragma pack(pop, mir)


NTSTATUS eusb_add_new_usb_device(
	_In_ WDFDEVICE hostControllerDevice
);

NTSTATUS
UsbCreateEndpointObj(
    _In_   UDECXUSBDEVICE    WdfUsbChildDevice,
    _In_   UCHAR             epAddr,
    _Out_  UDECXUSBENDPOINT* pNewEpObjAddr
);

NTSTATUS
Io_RetrieveEpQueue(
    _In_ UDECXUSBDEVICE  Device,
    _In_ UCHAR           EpAddr,
    _Out_ WDFQUEUE* Queue
);

static VOID
IoEvtControlUrb(
    _In_ WDFQUEUE Queue,
    _In_ WDFREQUEST Request,
    _In_ size_t OutputBufferLength,
    _In_ size_t InputBufferLength,
    _In_ ULONG IoControlCode
);

static VOID
IoEvtInterruptInUrb(
    _In_ WDFQUEUE Queue,
    _In_ WDFREQUEST Request,
    _In_ size_t OutputBufferLength,
    _In_ size_t InputBufferLength,
    _In_ ULONG IoControlCode
);

EVT_UDECX_USB_DEVICE_D0_ENTRY EvtUdecxUsbDeviceD0Entry;
EVT_UDECX_USB_DEVICE_D0_EXIT  EvtUdecxUsbDeviceD0Exit;
EVT_UDECX_USB_DEVICE_SET_FUNCTION_SUSPEND_AND_WAKE EvtUdecxUsbDeviceSetFunctionSuspendAndWake;

EVT_UDECX_USB_ENDPOINT_RESET  EvtUdecxUsbEndpointReset;

EXTERN_C_END