#pragma once
/*++

Module Name:

    device.h

Abstract:

    This file contains the device definitions.

Environment:

    Kernel-mode Driver Framework

--*/

#include "public.h"
#include "Queue.h"
#include <ntstrsafe.h>
#include <usb.h>
#include <wdfusb.h>
#include <usbdlib.h>
#include <ude/1.0/UdeCx.h>
#include <initguid.h>
#include <usbioctl.h>

EXTERN_C_START


#define USB_HOST_DEVINTERFACE_REF_STRING L"GUID_DEVINTERFACE_USB_HOST_CONTROLLER"

#define MAX_SUFFIX_SIZE                         (9 * sizeof(WCHAR)) // all ULONGs fit in 9 

#define BASE_DEVICE_NAME                        L"\\Device\\EUSB"
#define BASE_SYMBOLIC_LINK_NAME                 L"\\DosDevices\\EUSB"
#define DeviceNameSize                          (sizeof(BASE_DEVICE_NAME) + MAX_SUFFIX_SIZE)
#define SymLinkNameSize                         (sizeof(BASE_SYMBOLIC_LINK_NAME) + MAX_SUFFIX_SIZE)


typedef struct _REQUEST_CONTEXT {
    UINT32 unused;
} REQUEST_CONTEXT, * PREQUEST_CONTEXT;

WDF_DECLARE_CONTEXT_TYPE(REQUEST_CONTEXT);

typedef struct _HOST_CONTROLLER_CONTEXT
{
    KEVENT ResetCompleteEvent;
    WDFDEVICE* wdfSelfDevice;
    UNICODE_STRING DeviceName;

    WDFQUEUE DefaultQueue;
    WRITE_BUFFER_TO_READ_REQUEST_QUEUE missionRequest;
    WRITE_BUFFER_TO_READ_REQUEST_QUEUE missionCompletion;

    PUDECXUSBDEVICE_INIT  ChildDeviceInit;
    UDECXUSBDEVICE        ChildDevice;

} HOST_CONTROLLER_CONTEXT, *PHOST_CONTROLLER_CONTEXT;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(HOST_CONTROLLER_CONTEXT, GetHostControllerContext)

//
// Function to initialize the device and its callbacks
//
NTSTATUS
EUSBCreateHostControllerDevice(
    _Inout_ PWDFDEVICE_INIT DeviceInit
    );

EVT_WDF_OBJECT_CONTEXT_CLEANUP EvtWdfDeviceHostControllerContextCleanup;

EVT_WDF_DEVICE_PREPARE_HARDWARE EvtHostControllerPrepareHardware;
EVT_WDF_DEVICE_RELEASE_HARDWARE EvtHostControllerReleaseHardware;
EVT_WDF_DEVICE_D0_ENTRY EvtHostControllerD0Entry;
EVT_WDF_DEVICE_D0_EXIT EvtHostControllerD0Exit;
EVT_WDF_DEVICE_D0_ENTRY_POST_INTERRUPTS_ENABLED EvtHostControllerD0EntryPostInterruptsEnabled;
EVT_WDF_DEVICE_D0_EXIT_PRE_INTERRUPTS_DISABLED EvtHostControllerD0ExitPreInterruptsDisabled;

//EVT_UDECX_WDF_DEVICE_QUERY_USB_CAPABILITY ControllerEvtUdecxWdfDeviceQueryUsbCapability;

EXTERN_C_END
