#include "Driver.h"
#include "Device.h"
#include "Queue.tmh"

#include "USBDevice.h"


#define IOCTL_HID_MOVE_MOUSE \
    CTL_CODE(FILE_DEVICE_UNKNOWN, 0x880, METHOD_BUFFERED, FILE_ANY_ACCESS)


static VOID
_WQQCancelRequest(
    IN WDFQUEUE Queue,
    IN WDFREQUEST  Request
)
{
    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "[ INFO ] %!FUNC! Entry");
    UNREFERENCED_PARAMETER(Queue);
    WdfRequestComplete(Request, STATUS_CANCELLED);
}


static VOID
_WQQCancelUSBRequest(
    IN WDFQUEUE Queue,
    IN WDFREQUEST  Request
)
{
    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "[ INFO ] %!FUNC! Entry");
    UNREFERENCED_PARAMETER(Queue);
    LogInfo(TRACE_HC_DEVICE, "Canceling request %p", Request);
    UdecxUrbCompleteWithNtStatus(Request, STATUS_CANCELLED); 
}

VOID
WRQueueDestroy(
    _Inout_ PWRITE_BUFFER_TO_READ_REQUEST_QUEUE pQ
)
{
    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "[ INFO ] %!FUNC! Entry");
    PLIST_ENTRY e;
    if (pQ->qsync == NULL) {
        return; // init has not even started
    }

    // clean up the entire list
    WdfSpinLockAcquire(pQ->qsync);
    while ((e = RemoveHeadList(&(pQ->WriteBufferQueue))) != (&(pQ->WriteBufferQueue))) {
        PBUFFER_CONTENT pWriteEntry = CONTAINING_RECORD(e, BUFFER_CONTENT, BufferLink);
        ExFreePool(pWriteEntry);
    }
    WdfSpinLockRelease(pQ->qsync);

    WdfObjectDelete(pQ->ReadBufferQueue);
    pQ->ReadBufferQueue = NULL;

    WdfObjectDelete(pQ->qsync);
    pQ->qsync = NULL;
}

NTSTATUS
WRQueueInit(
    _In_    const WDFDEVICE* parent,
    _Inout_ PWRITE_BUFFER_TO_READ_REQUEST_QUEUE pQ,
    _In_    BOOLEAN bUSBReqQueue
)
{
    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "[ INFO ] %!FUNC! Entry");
    NTSTATUS status;
    WDF_IO_QUEUE_CONFIG queueConfig;

    memset(pQ, 0, sizeof(*pQ));

    status = STATUS_SUCCESS;
    WDF_IO_QUEUE_CONFIG_INIT(&queueConfig, WdfIoQueueDispatchManual);

    // when a request gets canceled, this is where we want to do the completion
    queueConfig.EvtIoCanceledOnQueue = (bUSBReqQueue ? _WQQCancelUSBRequest : _WQQCancelRequest);


    status = WdfSpinLockCreate(WDF_NO_OBJECT_ATTRIBUTES, &(pQ->qsync));
    if (!NT_SUCCESS(status)) {
        pQ->qsync = NULL;
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_QUEUE, "[ ERROR ] Unable to create spinlock, err= %!STATUS!", status);
        goto Error;
    }
    
    InitializeListHead(&(pQ->WriteBufferQueue));

    status = WdfIoQueueCreate(*parent,
        &queueConfig, WDF_NO_OBJECT_ATTRIBUTES, &(pQ->ReadBufferQueue));

    if (!NT_SUCCESS(status)) {
        pQ->ReadBufferQueue = NULL;
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_QUEUE, "[ ERROR ] Unable to create rd queue, err= %!STATUS!", status);
        goto Error;
    }

    goto Exit;

Error: // free anything done half-way
    WRQueueDestroy(pQ);
Exit:
    return status;
}

NTSTATUS InitCommandPipesQueue(const WDFDEVICE* wdfDevice)
{
    NTSTATUS status = STATUS_SUCCESS;
    PHOST_CONTROLLER_CONTEXT hostControllerContext;

    WDF_IO_QUEUE_CONFIG queueConfig;

    hostControllerContext = GetHostControllerContext(*wdfDevice);

    status = WRQueueInit(wdfDevice, &(hostControllerContext->missionRequest), FALSE);
    if (!NT_SUCCESS(status)) {
        LogError(TRACE_HC_DEVICE, "[ ERROR ] Unable to initialize mission completion, err= %!STATUS!", status);
        goto exit;
    }

    status = WRQueueInit(wdfDevice, &(hostControllerContext->missionCompletion), TRUE);
    if (!NT_SUCCESS(status)) {
        LogError(TRACE_HC_DEVICE, "[ ERROR ] Unable to initialize mission request, err= %!STATUS!", status);
        goto exit;
    }

    exit:
    return status;
}

void ControllerEvtIoDeviceControl(
    _In_ WDFQUEUE Queue,
    _In_ WDFREQUEST Request,
    _In_ size_t OutputBufferLength,
    _In_ size_t InputBufferLength,
    _In_ ULONG IoControlCode
)
{
    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "[ INFO ] %!FUNC! Entry/");
    NTSTATUS status = STATUS_SUCCESS;
    BOOLEAN handled;
    WDFREQUEST request;
    PUCHAR transferBuffer;
    ULONG transferBufferLength;
    WDFDEVICE ctrdevice = WdfIoQueueGetDevice(Queue);
    PHOST_CONTROLLER_CONTEXT hostControllerContext = GetHostControllerContext(ctrdevice);
    PUSB_CONTEXT usbDeviceContext = GetUsbDeviceContext(hostControllerContext->ChildDevice);



    handled = UdecxWdfDeviceTryHandleUserIoctl(ctrdevice,
        Request);


    if (!handled && IoControlCode == IOCTL_HID_MOVE_MOUSE)
    {
        LogInfo(TRACE_HC_DEVICE, "[ INFO ] IOCTL_HID_MOVE_MOUSE");

        unsigned char seq[4] = { 0x00, 0x0A, 0x0A, 0x00 };

        WdfRequestRetrieveInputBuffer(Request, 4, (PVOID*)&transferBuffer, (size_t*)&transferBufferLength);
        RtlCopyBytes(seq, transferBuffer, min(4, transferBufferLength));

        status = WdfIoQueueRetrieveNextRequest(usbDeviceContext->IntrDeferredQueue, &request);
        if (!NT_SUCCESS(status)) {
            UdecxUsbDeviceSignalWake(hostControllerContext->ChildDevice);
            LogInfo(TRACE_USB_DEVICE, "[ INFO ] WdfIoQueueRetrieveNextRequest empty...");
            WdfRequestComplete(Request, STATUS_SUCCESS);
            return;
        }

        status = UdecxUrbRetrieveBuffer(request, &transferBuffer, &transferBufferLength);
        if (!NT_SUCCESS(status))
        {
            LogError(TRACE_USB_DEVICE, "[ ERROR ] WdfRequest  %p unable to retrieve buffer %!STATUS!",
                Request, status);
            UdecxUrbCompleteWithNtStatus(Request, status);
            return;
        }

        LogInfo(TRACE_HC_DEVICE, "[ INFO ] retrive buff size: %d", transferBufferLength);

        RtlCopyBytes(transferBuffer, seq, 4);

        UdecxUrbSetBytesCompleted(request, 4);
        UdecxUrbCompleteWithNtStatus(request, STATUS_SUCCESS);

        WdfRequestComplete(Request, STATUS_SUCCESS);
        return;
    }

    if(!handled)
        WdfRequestComplete(Request, STATUS_SUCCESS);
}


void CommandPipeEvtRead(
    _In_ WDFQUEUE Queue,
    _In_ WDFREQUEST Request,
    _In_ size_t Length
)
{
    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "[ INFO ] %!FUNC! Entry");
    WdfRequestComplete(Request, STATUS_SUCCESS);
}


void CommandPipeEvtWrite(
    _In_ WDFQUEUE Queue,
    _In_ WDFREQUEST Request,
    _In_ size_t Length
)
{
    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "%!FUNC! Entry");
    NTSTATUS status;

    PVOID transferBuffer;
    SIZE_T transferBufferLength;

    PHOST_CONTROLLER_CONTEXT hostControllerContext;
    WDFDEVICE wdfHCDevice;

    wdfHCDevice = WdfIoQueueGetDevice(Queue);
    hostControllerContext = GetHostControllerContext(wdfHCDevice);


//https://docs.microsoft.com/en-us/windows-hardware/drivers/wdf/processing-i-o-requests

    // Retrive input buffer
    status = WdfRequestRetrieveInputBuffer(Request, 1, &transferBuffer, &transferBufferLength);
    if (!NT_SUCCESS(status))
    {
        LogError(TRACE_HC_DEVICE, "[ ERROR ] BCHAN WdfRequest write %p unable to retrieve buffer %!STATUS!", Request, status);
        goto exit;
    }

    // DEBUG TODO:
    DbgPrint("Recived: %s", transferBuffer);
    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "%s", transferBuffer);


exit:
    WdfRequestCompleteWithInformation(Request, status, transferBufferLength);
}