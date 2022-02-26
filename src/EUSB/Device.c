/*++

Module Name:

    device.c - Device handling events for example driver.

Abstract:

   This file contains the device entry points and callbacks.
    
Environment:

    Kernel-mode Driver Framework

--*/

#include "driver.h"
#include "device.tmh"

#include "USBDevice.h"


#ifdef ALLOC_PRAGMA
#pragma alloc_text (PAGE, EUSBCreateHostControllerDevice)
#endif

void EvtWdfDeviceHostControllerContextCleanup(_In_ WDFOBJECT wdfDevice)
{
    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "[ INFO ] %!FUNC! Entry context cleanup");
    PHOST_CONTROLLER_CONTEXT        hostControllerContext;
    hostControllerContext = GetHostControllerContext(wdfDevice);

    if (hostControllerContext->DeviceName.Buffer != NULL){
        ExFreePoolWithTag(hostControllerContext->DeviceName.Buffer, EUSB_BUFF_TAG);
    }

    // cleanup context....
}

///////////////////////////////////////////////////////////////////////////////////////////
////////////  PNP POWER  //////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////

NTSTATUS EvtHostControllerPrepareHardware(
    _In_ WDFDEVICE Device,
    _In_ WDFCMRESLIST ResourcesRaw,
    _In_ WDFCMRESLIST ResourcesTranslated
)
{
    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "[ INFO ] %!FUNC! Entry");
    UNREFERENCED_PARAMETER(Device);
    UNREFERENCED_PARAMETER(ResourcesRaw);
    UNREFERENCED_PARAMETER(ResourcesTranslated);

    return STATUS_SUCCESS;
}


NTSTATUS EvtHostControllerReleaseHardware(
    _In_ WDFDEVICE Device,
    _In_ WDFCMRESLIST ResourcesTranslated
)
{
    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "[ INFO ] %!FUNC! Entry");
    UNREFERENCED_PARAMETER(Device);
    UNREFERENCED_PARAMETER(ResourcesTranslated);
    return STATUS_SUCCESS;
}


NTSTATUS EvtHostControllerD0Entry(
    _In_ WDFDEVICE hostControllerDevice,
    _In_ WDF_POWER_DEVICE_STATE PreviousState
)
{
    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "[ INFO ] %!FUNC! Entry");
    
    NTSTATUS status = STATUS_SUCCESS;
    PHOST_CONTROLLER_CONTEXT hostControllerContext;

    hostControllerContext = GetHostControllerContext(hostControllerDevice);

    if (PreviousState == WdfPowerDeviceD3Final)
    {
        // now we can init usb endpoints, and then plug in them
    }
    return status;
}


NTSTATUS EvtHostControllerD0Exit(
    _In_ WDFDEVICE Device,
    _In_ WDF_POWER_DEVICE_STATE TargetState
)
{
    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "[ INFO ] %!FUNC! Entry");
    UNREFERENCED_PARAMETER(Device);
    UNREFERENCED_PARAMETER(TargetState);

    return STATUS_SUCCESS;
}


NTSTATUS EvtHostControllerD0EntryPostInterruptsEnabled(
    _In_ WDFDEVICE Device,
    _In_ WDF_POWER_DEVICE_STATE PreviousState
)
{
    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "[ INFO ] %!FUNC! Entry");
    UNREFERENCED_PARAMETER(Device);
    UNREFERENCED_PARAMETER(PreviousState);
    return STATUS_SUCCESS;
}


NTSTATUS EvtHostControllerD0ExitPreInterruptsDisabled(
    _In_ WDFDEVICE Device,
    _In_ WDF_POWER_DEVICE_STATE TargetState
)
{
    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "[ INFO ] %!FUNC! Entry");
    UNREFERENCED_PARAMETER(Device);
    UNREFERENCED_PARAMETER(TargetState);
    return STATUS_SUCCESS;
}


/// <summary>
/// Возможности хост контроллера
/// </summary>
/// <param name="UdecxWdfDevice"></param>
/// <param name="CapabilityType"></param>
/// <param name="OutputBufferLength"></param>
/// <param name="OutputBuffer"></param>
/// <param name="ResultLength"></param>
/// <returns></returns>
NTSTATUS ControllerEvtUdecxWdfDeviceQueryUsbCapability(
    _In_            WDFDEVICE UdecxWdfDevice,
    _In_            PGUID CapabilityType,
    _In_            ULONG OutputBufferLength,
    _Out_opt_       PVOID OutputBuffer,
    _Out_           PULONG ResultLength
)
{
    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "[ INFO ] %!FUNC! Entry");
    UNREFERENCED_PARAMETER(UdecxWdfDevice);
    UNREFERENCED_PARAMETER(OutputBufferLength);
    
    OutputBuffer = NULL;
    ResultLength = 0;

    if (RtlCompareMemory(
        CapabilityType,
        &GUID_USB_CAPABILITY_DEVICE_CONNECTION_HIGH_SPEED_COMPATIBLE,
        sizeof(GUID)
    ) == sizeof(GUID))
    {
        return STATUS_SUCCESS;
    }
    return STATUS_UNSUCCESSFUL;
}


NTSTATUS
EUSBCreateHostControllerDevice(
    _Inout_ PWDFDEVICE_INIT DeviceInit
    )
/*++

Routine Description:

    Worker routine called to create a virtual host controller device and its software resources.

Arguments:

    DeviceInit - Pointer to an opaque init structure. Memory for this
                    structure will be freed by the framework when the WdfDeviceCreate
                    succeeds. So don't access the structure after that point.

Return Value:

    NTSTATUS

--*/
{
    PAGED_CODE();
    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "[ INFO ] %!FUNC! Entry");

    NTSTATUS                        status = STATUS_SUCCESS;

    WDFDEVICE                       wdfHCDevice;                    // Virtual host controller device
    WDF_OBJECT_ATTRIBUTES           hostControllerAttributes;       //    attributes
    WDF_OBJECT_ATTRIBUTES           hostControllerRequestAttributes;//    request attributes
    PHOST_CONTROLLER_CONTEXT        hostControllerContext = NULL;   //    context
    WDF_PNPPOWER_EVENT_CALLBACKS    devicePnpPowerCallbacks;        //    power callbacks
    UDECX_WDF_DEVICE_CONFIG         udecxConfig;
    WDF_FILEOBJECT_CONFIG           fileConfig;                     // file object config

    WDF_IO_QUEUE_CONFIG                    defaultQueueConfig;
    WDF_DEVICE_POWER_POLICY_IDLE_SETTINGS  idleSettings;

    UNICODE_STRING                  refString;
        
    
    WDF_PNPPOWER_EVENT_CALLBACKS_INIT(&devicePnpPowerCallbacks);
    devicePnpPowerCallbacks.EvtDevicePrepareHardware = EvtHostControllerPrepareHardware;
    devicePnpPowerCallbacks.EvtDeviceReleaseHardware = EvtHostControllerReleaseHardware;
    devicePnpPowerCallbacks.EvtDeviceD0Entry = EvtHostControllerD0Entry;
    devicePnpPowerCallbacks.EvtDeviceD0Exit = EvtHostControllerD0Exit;
    devicePnpPowerCallbacks.EvtDeviceD0EntryPostInterruptsEnabled = EvtHostControllerD0EntryPostInterruptsEnabled;
    devicePnpPowerCallbacks.EvtDeviceD0ExitPreInterruptsDisabled = EvtHostControllerD0ExitPreInterruptsDisabled;

    WdfDeviceInitSetPnpPowerEventCallbacks(DeviceInit, &devicePnpPowerCallbacks);

    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&hostControllerRequestAttributes, REQUEST_CONTEXT);
    WdfDeviceInitSetRequestAttributes(DeviceInit, &hostControllerRequestAttributes);

    WDF_FILEOBJECT_CONFIG_INIT(&fileConfig,
        WDF_NO_EVENT_CALLBACK,
        WDF_NO_EVENT_CALLBACK,
        WDF_NO_EVENT_CALLBACK // No cleanup callback function
    );

    // WdfDeviceInitSetIoType(DeviceInit, WdfDeviceIoBuffered); // TODO:

    //
    // Safest value forces WDF to track handles separately. If the driver stack allows it, then
    // for performance, we should change this to a different option.
    //
    fileConfig.FileObjectClass = WdfFileObjectWdfCannotUseFsContexts;
    WdfDeviceInitSetFileObjectConfig(DeviceInit,
        &fileConfig,
        WDF_NO_OBJECT_ATTRIBUTES
    );

    //
    // Set the security descriptor for the device.
    //
    status = WdfDeviceInitAssignSDDLString(DeviceInit, &SDDL_DEVOBJ_SYS_ALL_ADM_RWX_WORLD_RW_RES_R);
    if (!NT_SUCCESS(status)) {

        LogError(TRACE_HC_DEVICE, "[ ERROR ] WdfDeviceInitAssignSDDLString Failed %!STATUS!", status);
        goto exit;
    }

    //
    // Do additional setup required for USB controllers.
    //
    status = UdecxInitializeWdfDeviceInit(DeviceInit);
    if (!NT_SUCCESS(status)) {
        LogError(TRACE_HC_DEVICE, "[ ERROR ] UdecxInitializeDeviceInit failed %!STATUS!", status);
        goto exit;
    }

    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&hostControllerAttributes, HOST_CONTROLLER_CONTEXT);
    hostControllerAttributes.EvtCleanupCallback = EvtWdfDeviceHostControllerContextCleanup;

    //
    // Create Device
    //
    DECLARE_UNICODE_STRING_SIZE(uDeviceName, DeviceNameSize);
    DECLARE_UNICODE_STRING_SIZE(uSymLinkName, SymLinkNameSize);

    ULONG i = 0;
    for (i = 0; i < MAXUINT32; ++i)
    {
        // As result '\\Device\\EUSB' || 'i'
        status = RtlUnicodeStringPrintf(&uDeviceName, L"%ws%d", BASE_DEVICE_NAME, i);
        if (!NT_SUCCESS(status)) {
            LogError(TRACE_HC_DEVICE, "[ ERROR ] RtlUnicodeStringPrintf (uniDeviceName) failed %!STATUS!", status);
            goto exit;
        }

        status = WdfDeviceInitAssignName(DeviceInit, &uDeviceName);
        if (!NT_SUCCESS(status)) {
            LogError(TRACE_HC_DEVICE, "[ ERROR ] WdfDeviceInitAssignName Failed %!STATUS!", status);
            goto exit;
        }

        status = WdfDeviceCreate(&DeviceInit, &hostControllerAttributes, &wdfHCDevice);

        if (status == STATUS_OBJECT_NAME_COLLISION){
            LogVerbose(TRACE_HC_DEVICE, "[ WARNING ] WdfDeviceCreate Object Name Collision %d", i);
        }
        else if (!NT_SUCCESS(status)) {
            LogError(TRACE_HC_DEVICE, "[ ERROR ] WdfDeviceCreate Failed %!STATUS!", status);
            goto exit;
        }
        else {
            LogInfo(TRACE_HC_DEVICE, "[ OK ] WdfDeviceCreate is success.");

            hostControllerContext = GetHostControllerContext(wdfHCDevice);
            hostControllerContext->wdfSelfDevice = &wdfHCDevice;

            hostControllerContext->DeviceName.Length = hostControllerContext->DeviceName.MaximumLength = wcslen(uDeviceName_buffer);
            hostControllerContext->DeviceName.Buffer = ExAllocatePoolWithTag(PagedPool, hostControllerContext->DeviceName.Length, EUSB_BUFF_TAG);
            // TODO: проверить чтобы функция копрования не выделяла новый буфер
            RtlCopyUnicodeString(&hostControllerContext->DeviceName, &uDeviceName);
            break;
        }
    }

    if (hostControllerContext == NULL)
    {
        LogError(TRACE_HC_DEVICE, "[ ERROR ] WdfDeviceCreate Failed %!STATUS!", status);
        goto exit;
    }

    //
    // Create the symbolic link (also for compatibility).
    //
    status = RtlUnicodeStringPrintf(&uSymLinkName, L"%ws%d", BASE_SYMBOLIC_LINK_NAME, i);
    if (!NT_SUCCESS(status)) {

        LogError(TRACE_HC_DEVICE, "[ ERROR ] RtlUnicodeStringPrintf (SymLinkName) Failed %!STATUS!", status);
        goto exit;
    }

    status = WdfDeviceCreateSymbolicLink(wdfHCDevice, &uSymLinkName);
    if (!NT_SUCCESS(status)) {

        LogError(TRACE_HC_DEVICE, "[ ERROR ] WdfDeviceCreateSymbolicLink Failed %!STATUS!", status);
        goto exit;
    }

    //
    // Create the device interface.
    //
    RtlInitUnicodeString(&refString,
        USB_HOST_DEVINTERFACE_REF_STRING);

    status = WdfDeviceCreateDeviceInterface(wdfHCDevice,
        (LPGUID)&GUID_DEVINTERFACE_USB_HOST_CONTROLLER,
        &refString);
    if (!NT_SUCCESS(status)) {

        LogError(TRACE_HC_DEVICE, "[ ERROR ] WdfDeviceCreateDeviceInterface Failed %!STATUS!", status);
        goto exit;
    }


    // create a 2nd interface for back-channel interaction with the controller
    status = WdfDeviceCreateDeviceInterface(wdfHCDevice,
        (LPGUID)&GUID_DEVINTERFACE_EUSB,
        NULL);

    if (!NT_SUCCESS(status)) {

        LogError(TRACE_HC_DEVICE, "[ ERROR ] WdfDeviceCreateDeviceInterface (backchannel) Failed %!STATUS!", status);
        goto exit;
    }

    UDECX_WDF_DEVICE_CONFIG_INIT(&udecxConfig, ControllerEvtUdecxWdfDeviceQueryUsbCapability);
    status = UdecxWdfDeviceAddUsbDeviceEmulation(wdfHCDevice,
        &udecxConfig);
    if (!NT_SUCCESS(status)) {
        LogError(TRACE_HC_DEVICE, "[ ERROR ] Unable to add USB device emulation, err= %!STATUS!", status);
        goto exit;
    }

    status = InitCommandPipesQueue(&wdfHCDevice);
    if (!NT_SUCCESS(status))
    {
        LogError(TRACE_HC_DEVICE, "[ ERROR ] Unable to initialize backchannel err=%!STATUS!", status);
        goto exit;
    }

    KeInitializeEvent(&(hostControllerContext->ResetCompleteEvent),
        NotificationEvent,
        FALSE /* initial state: not signaled */
    );

    //
    //       Create default queue. It only supports USB controller IOCTLs. (USB I/O will come through
    // in separate USB device queues.)
    //      The I/O queue's requests are presented to the driver's request handlers one at a time. 
    // The framework does not deliver the next request until a driver has called WdfRequestComplete 
    // to complete the current request.
    //       If your driver creates a default I/O queue, the framework places all of the device's I/O 
    // requests in this queue, unless you create additional queues to receive 
    // some of the requests. A driver can obtain a handle to a 
    // device's default I/O queue by the WdfDeviceGetDefaultQueue method.
    //
    WDF_IO_QUEUE_CONFIG_INIT_DEFAULT_QUEUE(&defaultQueueConfig, WdfIoQueueDispatchSequential);
    defaultQueueConfig.EvtIoDeviceControl = ControllerEvtIoDeviceControl;
    defaultQueueConfig.EvtIoRead = CommandPipeEvtRead;
    defaultQueueConfig.EvtIoWrite = CommandPipeEvtWrite;
    defaultQueueConfig.PowerManaged = WdfFalse;
    
    status = WdfIoQueueCreate(wdfHCDevice,
        &defaultQueueConfig,
        WDF_NO_OBJECT_ATTRIBUTES,
        &(hostControllerContext->DefaultQueue));
    if (!NT_SUCCESS(status)) {
        LogError(TRACE_HC_DEVICE, "[ ERROR ] Default queue creation failed %!STATUS!", status);
        goto exit;
    }

    eusb_add_new_usb_device(wdfHCDevice);

    WDF_DEVICE_POWER_POLICY_IDLE_SETTINGS_INIT(&idleSettings, IdleCannotWakeFromS0); // TODO:can

    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "[ OK ] %!FUNC! Ret");

exit:

    return status;
}
