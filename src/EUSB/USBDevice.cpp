#include "USBDevice.h"
#include "USBDevice.tmh"

#include "driver.h"

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


NTSTATUS eusb_add_new_usb_device(
	_In_ WDFDEVICE hostControllerDevice
)
{   
    LogInfo(TRACE_HC_DEVICE, "[ INFO ] %!FUNC! Entry");
    NTSTATUS                                    status = STATUS_SUCCESS;
    PHOST_CONTROLLER_CONTEXT                    hostControllerContext;

    PUDECXUSBDEVICE_INIT                        usbDeviceInit;
    UDECX_USB_DEVICE_STATE_CHANGE_CALLBACKS     callbacks;

    const USHORT UsbDeviceDescriptorLen = sizeof(g_UsbDeviceDescriptor);
    const USHORT UsbConfigurationDescriptorLen = sizeof(g_UsbConfigDescriptorSet);
    PUCHAR UsbDeviceDescriptor = (PUCHAR)ExAllocatePoolWithTag(NonPagedPoolNx, UsbDeviceDescriptorLen, EUSB_BUFF_TAG);
    PUCHAR UsbConfigurationDescriptor = (PUCHAR)ExAllocatePoolWithTag(NonPagedPoolNx, UsbConfigurationDescriptorLen, EUSB_BUFF_TAG);
    
    if (UsbDeviceDescriptor == NULL ||
        UsbConfigurationDescriptor == NULL) {
        status = STATUS_MEMORY_NOT_ALLOCATED;
        goto exit;
    }
    
    RtlCopyBytes(UsbDeviceDescriptor, g_UsbDeviceDescriptor, UsbDeviceDescriptorLen);
    RtlCopyBytes(UsbConfigurationDescriptor, g_UsbConfigDescriptorSet, UsbConfigurationDescriptorLen);

    hostControllerContext = GetHostControllerContext(hostControllerDevice);

    usbDeviceInit = UdecxUsbDeviceInitAllocate(hostControllerDevice);
    hostControllerContext->ChildDeviceInit = usbDeviceInit;

    if (usbDeviceInit == NULL) {
        status = STATUS_INSUFFICIENT_RESOURCES;
        LogError(TRACE_HC_DEVICE, "[ ERROR ] Failed to allocate UDECXUSBDEVICE_INIT %!STATUS!", status);
        goto exit;
    }

    UDECX_USB_DEVICE_CALLBACKS_INIT(&callbacks);
    // TODO:
    callbacks.EvtUsbDeviceLinkPowerEntry = EvtUdecxUsbDeviceD0Entry;
    callbacks.EvtUsbDeviceLinkPowerExit = EvtUdecxUsbDeviceD0Exit;
    callbacks.EvtUsbDeviceSetFunctionSuspendAndWake = EvtUdecxUsbDeviceSetFunctionSuspendAndWake;

    UdecxUsbDeviceInitSetStateChangeCallbacks(usbDeviceInit, &callbacks);

    UdecxUsbDeviceInitSetSpeed(usbDeviceInit, UdecxUsbHighSpeed);
    UdecxUsbDeviceInitSetEndpointsType(usbDeviceInit, UdecxEndpointTypeSimple);

    status = UdecxUsbDeviceInitAddDescriptor(usbDeviceInit,
        (PUCHAR)UsbDeviceDescriptor,
        UsbDeviceDescriptorLen);
    if (!NT_SUCCESS(status)) {
        LogError(TRACE_HC_DEVICE, "[ ERROR ] UdecxUsbDeviceInitAddDescriptor %!STATUS!", status);
        goto exit;
    }

    status = UdecxUsbDeviceInitAddDescriptorWithIndex(usbDeviceInit,
        (PUCHAR)g_LanguageDescriptor,
        sizeof(g_LanguageDescriptor),
        0);
    if (!NT_SUCCESS(status)) {
        LogError(TRACE_HC_DEVICE, "[ ERROR ] UdecxUsbDeviceInitAddDescriptorWithIndex %!STATUS!", status);
        goto exit;
    }

    status = UdecxUsbDeviceInitAddStringDescriptor(usbDeviceInit,
        &g_ManufacturerStringEnUs,
        g_ManufacturerIndex,
        AMERICAN_ENGLISH);
    if (!NT_SUCCESS(status)) {
        LogError(TRACE_HC_DEVICE, "[ ERROR ] UdecxUsbDeviceInitAddStringDescriptor %!STATUS!", status);
        goto exit;
    }

    status = UdecxUsbDeviceInitAddStringDescriptor(usbDeviceInit,
        &g_ProductStringEnUs,
        g_ProductIndex,
        AMERICAN_ENGLISH);
    if (!NT_SUCCESS(status)) {
        LogError(TRACE_HC_DEVICE, "[ ERROR ] UdecxUsbDeviceInitAddStringDescriptor %!STATUS!", status);
        goto exit;
    }

    status = UdecxUsbDeviceInitAddDescriptor(usbDeviceInit,
        (PUCHAR)UsbConfigurationDescriptor,
        UsbConfigurationDescriptorLen);
    if (!NT_SUCCESS(status)) {
        LogError(TRACE_HC_DEVICE, "[ ERROR ] UdecxUsbDeviceInitAddDescriptor %!STATUS!", status);
        goto exit;
    }

    LogInfo(TRACE_HC_DEVICE, "[ OK ] %!FUNC! descriptors was initialized.");


    // ############################################################################3
    PUSB_CONTEXT                      deviceContext = NULL;
    WDF_OBJECT_ATTRIBUTES             attributes;
    UDECX_USB_DEVICE_PLUG_IN_OPTIONS  pluginOptions;

    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&attributes, USB_CONTEXT);

    status = UdecxUsbDeviceCreate(&usbDeviceInit,
        &attributes,
        &(hostControllerContext->ChildDevice));

    if (!NT_SUCCESS(status)) {
        LogError(TRACE_HC_DEVICE, "[ ERROR ] UdecxUsbDeviceCreate failed with status %!STATUS!", status);
        goto exit;
    }

    deviceContext = GetUsbDeviceContext(hostControllerContext->ChildDevice);
    deviceContext->ControllerDevice = hostControllerDevice;

    //
    // Create static endpoints.
    //
    status = UsbCreateEndpointObj(hostControllerContext->ChildDevice,
        USB_DEFAULT_ENDPOINT_ADDRESS,
        &(deviceContext->UDEFX2ControlEndpoint));
    if (!NT_SUCCESS(status)) {
        LogError(TRACE_HC_DEVICE, "[ ERROR ] UsbCreateEndpointObj failed with status %!STATUS!", status);
        goto exit;
    }

    status = UsbCreateEndpointObj(hostControllerContext->ChildDevice,
        g_InterruptEndpointAddress,
        &(deviceContext->UDEFX2InterruptInEndpoint));
    if (!NT_SUCCESS(status)) {
        LogError(TRACE_HC_DEVICE, "[ ERROR ] UsbCreateEndpointObj failed with status %!STATUS!", status);
        goto exit;
    }

    UDECX_USB_DEVICE_PLUG_IN_OPTIONS_INIT(&pluginOptions);
    pluginOptions.Usb20PortNumber = 1;
    status = UdecxUsbDevicePlugIn(hostControllerContext->ChildDevice, &pluginOptions);

    LogInfo(TRACE_HC_DEVICE, "[ OK ] usb init ends successfully!");

exit:
    if (UsbDeviceDescriptor != NULL)
        ExFreePoolWithTag(UsbDeviceDescriptor, EUSB_BUFF_TAG);
    if (UsbConfigurationDescriptor != NULL)
        ExFreePoolWithTag(UsbConfigurationDescriptor, EUSB_BUFF_TAG);

	return status;
}

NTSTATUS
UsbCreateEndpointObj(
    _In_   UDECXUSBDEVICE    WdfUsbChildDevice,
    _In_   UCHAR             epAddr,
    _Out_  UDECXUSBENDPOINT* pNewEpObjAddr
)
{
    NTSTATUS                      status;
    PUSB_CONTEXT                  pUsbContext;
    WDFQUEUE                      epQueue;
    UDECX_USB_ENDPOINT_CALLBACKS  callbacks;
    PUDECXUSBENDPOINT_INIT        endpointInit;


    pUsbContext = GetUsbDeviceContext(WdfUsbChildDevice);
    endpointInit = NULL;

    status = Io_RetrieveEpQueue(WdfUsbChildDevice, epAddr, &epQueue);

    if (!NT_SUCCESS(status)) {
        goto exit;
    }

    endpointInit = UdecxUsbSimpleEndpointInitAllocate(WdfUsbChildDevice);

    if (endpointInit == NULL) {

        status = STATUS_INSUFFICIENT_RESOURCES;
        LogError(TRACE_HC_DEVICE, "[ ERROR ] Failed to allocate endpoint init %!STATUS!", status);
        goto exit;
    }

    UdecxUsbEndpointInitSetEndpointAddress(endpointInit, epAddr);

    UDECX_USB_ENDPOINT_CALLBACKS_INIT(&callbacks, EvtUdecxUsbEndpointReset);
    UdecxUsbEndpointInitSetCallbacks(endpointInit, &callbacks);

    status = UdecxUsbEndpointCreate(&endpointInit,
        WDF_NO_OBJECT_ATTRIBUTES,
        pNewEpObjAddr);

    if (!NT_SUCCESS(status)) {

        LogError(TRACE_HC_DEVICE, "[ ERROR ] UdecxUsbEndpointCreate failed for endpoint %x, %!STATUS!", epAddr, status);
        goto exit;
    }

    // TODO: как это работает, как она перенаправляет URB в нужную очередь??
    UdecxUsbEndpointSetWdfIoQueue(*pNewEpObjAddr, epQueue);

exit:

    if (endpointInit != NULL) {

        NT_ASSERT(!NT_SUCCESS(status));
        UdecxUsbEndpointInitFree(endpointInit);
        endpointInit = NULL;
    }

    return status;
}

NTSTATUS
Io_RetrieveEpQueue(
    _In_ UDECXUSBDEVICE  Device,
    _In_ UCHAR           EpAddr,
    _Out_ WDFQUEUE* Queue
)
{
    NTSTATUS status;
    PUSB_CONTEXT pUsbContext;
    WDF_IO_QUEUE_CONFIG queueConfig;
    WDFDEVICE wdfController;
    WDFQUEUE* pQueueRecord = NULL;
    WDF_OBJECT_ATTRIBUTES  attributes;
    PFN_WDF_IO_QUEUE_IO_INTERNAL_DEVICE_CONTROL pIoCallback = NULL;

    status = STATUS_SUCCESS;
    pUsbContext = GetUsbDeviceContext(Device);

    wdfController = pUsbContext->ControllerDevice;

    switch (EpAddr)
    {
    case USB_DEFAULT_ENDPOINT_ADDRESS:
        pQueueRecord = &(pUsbContext->ControlQueue);
        pIoCallback = IoEvtControlUrb;
        break;
    case g_InterruptEndpointAddress:
        pQueueRecord = &(pUsbContext->InterruptUrbQueue);
        pIoCallback = IoEvtInterruptInUrb;

        {
            WDF_IO_QUEUE_CONFIG __queueConfig;
            WDF_IO_QUEUE_CONFIG_INIT(&__queueConfig, WdfIoQueueDispatchManual);

            // when a request gets canceled, this is where we want to do the completion
            /// __queueConfig.EvtIoCanceledOnQueue = IoEvtCancelInterruptInUrb;

            //
            // We shouldn't have to power-manage this queue, as we will manually 
            // purge it and de-queue from it whenever we get power indications.
            //
            __queueConfig.PowerManaged = WdfFalse;

            status = WdfIoQueueCreate(wdfController,
                &__queueConfig,
                WDF_NO_OBJECT_ATTRIBUTES,
                &(pUsbContext->IntrDeferredQueue)
            );
        }

        break;

    default:
        LogError(TRACE_USB_DEVICE, "[ ERROR ] Io_RetrieveEpQueue received unrecognized ep %x", EpAddr);
        status = STATUS_ILLEGAL_FUNCTION;
        goto exit;
    }


    *Queue = NULL;
    if (!NT_SUCCESS(status)) {
        goto exit;
    }

    if ((*pQueueRecord) == NULL) {
        WDF_IO_QUEUE_CONFIG_INIT(&queueConfig, WdfIoQueueDispatchSequential);

        //Sequential must specify this callback
        queueConfig.EvtIoInternalDeviceControl = pIoCallback;

        status = WdfIoQueueCreate(wdfController,
            &queueConfig,
            WDF_NO_OBJECT_ATTRIBUTES,
            pQueueRecord);

        if (!NT_SUCCESS(status)) {
            LogError(TRACE_USB_DEVICE, "[ ERROR ] WdfIoQueueCreate failed for queue of ep %x %!STATUS!", EpAddr, status);
            goto exit;
        }
    }

    *Queue = (*pQueueRecord);

exit:

    return status;
}

NTSTATUS CompleteRequestWithDescriptor(
    _In_ WDFREQUEST Request,
    const _In_ PUCHAR Descriptor,
    const _In_ ULONG DescriptorLength
)
{
    PUCHAR buffer;
    ULONG length = 0;
    NTSTATUS status = UdecxUrbRetrieveBuffer(Request, &buffer, &length);

    if (!NT_SUCCESS(status)) {
        LogError(TRACE_USB_DEVICE, "Can't get buffer for descriptor: %!STATUS!", status);
        return status;
    }

    if (length < DescriptorLength) {
        LogError(TRACE_USB_DEVICE, "Buffer too small");
        return STATUS_BUFFER_TOO_SMALL;
    }

    memcpy(buffer, Descriptor, DescriptorLength);

    return STATUS_SUCCESS;
}

static VOID
IoEvtControlUrb(
    _In_ WDFQUEUE Queue,
    _In_ WDFREQUEST Request,
    _In_ size_t OutputBufferLength,
    _In_ size_t InputBufferLength,
    _In_ ULONG IoControlCode
)
{
    WDF_USB_CONTROL_SETUP_PACKET setupPacket;
    NTSTATUS status;
    UNREFERENCED_PARAMETER(Queue);
    UNREFERENCED_PARAMETER(OutputBufferLength);
    UNREFERENCED_PARAMETER(InputBufferLength);

    // PENDPOINTQUEUE_CONTEXT pEpQContext = GetEndpointQueueContext(Queue);
     //WDFDEVICE backchannel = pEpQContext->backChannelDevice; // HC
     //PUDECX_BACKCHANNEL_CONTEXT pBackChannelContext = GetBackChannelContext(backchannel);

     //NT_VERIFY(IoControlCode == IOCTL_INTERNAL_USB_SUBMIT_URB);

    if (IoControlCode == IOCTL_INTERNAL_USB_SUBMIT_URB)
    {
        // These are on the control pipe.
        // We don't do anything special with these requests,
        // but this is where we would add processing for vendor-specific commands.

        status = UdecxUrbRetrieveControlSetupPacket(Request, &setupPacket);

        if (!NT_SUCCESS(status))
        {
            LogError(TRACE_USB_DEVICE, "WdfRequest %p is not a control URB? UdecxUrbRetrieveControlSetupPacket %!STATUS!",
                Request, status);
            UdecxUrbCompleteWithNtStatus(Request, status);
            goto exit;
        }

        if (setupPacket.Packet.bRequest == 0x06) {
            //  descriptor type is report descriptor (0x22)
            if (setupPacket.Packet.wValue.Bytes.HiByte == 0x22) {
                LogInfo(TRACE_USB_DEVICE, "[IoEvtControlUrb] Report descriptor is requested");
                status = CompleteRequestWithDescriptor(Request, (PUCHAR)g_HIDMouseUsbReportDescriptor, 0x3E);
                UdecxUrbCompleteWithNtStatus(Request, status);
                goto exit;
            }

            // descriptor type is BOS descriptor (0x0F)
            /*if (setupPacket.Packet.wValue.Bytes.HiByte == 0x0F) {
                LogInfo(TRACE_DEVICE, "[IoEvtControlUrb] BOS descriptor is requested");
                // TODO:
                // now we have only one BOS descriptor which will be send to each BOS request, so we just use it
                // This is dirty and fast solution and need refactoring

                status = CompleteRequestWithDescriptor(Request, (PUCHAR)g_BOS, 22);
                UdecxUrbCompleteWithNtStatus(Request, status);
                goto exit;
            }*/
        }


        LogInfo(TRACE_USB_DEVICE, "v44 control CODE %x, [type=%x dir=%x recip=%x] req=%x [wv = %x wi = %x wlen = %x]",
            IoControlCode,
            (int)(setupPacket.Packet.bm.Request.Type),
            (int)(setupPacket.Packet.bm.Request.Dir),
            (int)(setupPacket.Packet.bm.Request.Recipient),
            (int)(setupPacket.Packet.bRequest),
            (int)(setupPacket.Packet.wValue.Value),
            (int)(setupPacket.Packet.wIndex.Value),
            (int)(setupPacket.Packet.wLength)
        );

        UdecxUrbCompleteWithNtStatus(Request, STATUS_SUCCESS);
    }
    else
    {
        LogError(TRACE_USB_DEVICE, "control NO submit code is %x", IoControlCode);
    }
exit:
    return;
}

static VOID
IoEvtInterruptInUrb(
    _In_ WDFQUEUE Queue,
    _In_ WDFREQUEST Request,
    _In_ size_t OutputBufferLength,
    _In_ size_t InputBufferLength,
    _In_ ULONG IoControlCode
)
{
    static int req_index = 0;
    ++req_index;
    LogInfo(TRACE_USB_DEVICE, "[ URB INTERRUPT ] %!FUNC! Entr %d", req_index);

    UDECXUSBDEVICE tgtDevice;
    NTSTATUS status = STATUS_SUCCESS;
    PUCHAR transferBuffer;
    ULONG transferBufferLength;

    UNREFERENCED_PARAMETER(Request);
    UNREFERENCED_PARAMETER(OutputBufferLength);
    UNREFERENCED_PARAMETER(InputBufferLength);

    WDFDEVICE ctrdevice = WdfIoQueueGetDevice(Queue);
    PHOST_CONTROLLER_CONTEXT hostControllerContext = GetHostControllerContext(ctrdevice);
    PUSB_CONTEXT usbDeviceContext = GetUsbDeviceContext(hostControllerContext->ChildDevice);

    status = WdfRequestForwardToIoQueue(Request, usbDeviceContext->IntrDeferredQueue);
    if (NT_SUCCESS(status)) {

       LogInfo(TRACE_USB_DEVICE, "Request %p forwarded for later", Request);
    }
    else {
        LogError(TRACE_USB_DEVICE, "ERROR: Unable to forward Request %p error %!STATUS!", Request, status);
        UdecxUrbCompleteWithNtStatus(Request, status);
    }
    return;
}

NTSTATUS
EvtUdecxUsbDeviceD0Entry(
    _In_ WDFDEVICE       UdecxWdfDevice,
    _In_ UDECXUSBDEVICE    UdecxUsbDevice)
{
    UNREFERENCED_PARAMETER(UdecxWdfDevice);
    LogInfo(TRACE_USB_DEVICE, "[ INFO ] %!FUNC! ");
    return STATUS_SUCCESS;
}

NTSTATUS
EvtUdecxUsbDeviceD0Exit(
    _In_ WDFDEVICE UdecxWdfDevice,
    _In_ UDECXUSBDEVICE UdecxUsbDevice,
    _In_ UDECX_USB_DEVICE_WAKE_SETTING WakeSetting)
{
    UNREFERENCED_PARAMETER(UdecxWdfDevice);
    LogInfo(TRACE_USB_DEVICE, "[ INFO ] %!FUNC! USB Device power EXIT [wdfDev=%p, usbDev=%p], WakeSetting=%x", UdecxWdfDevice, UdecxUsbDevice, WakeSetting);
    return STATUS_SUCCESS;
}

NTSTATUS
EvtUdecxUsbDeviceSetFunctionSuspendAndWake(
    _In_ WDFDEVICE                        UdecxWdfDevice,
    _In_ UDECXUSBDEVICE                   UdecxUsbDevice,
    _In_ ULONG                            Interface,
    _In_ UDECX_USB_DEVICE_FUNCTION_POWER  FunctionPower
)
{
    UNREFERENCED_PARAMETER(UdecxWdfDevice);
    UNREFERENCED_PARAMETER(UdecxUsbDevice);
    UNREFERENCED_PARAMETER(Interface);
    UNREFERENCED_PARAMETER(FunctionPower);

    // this never gets printed!
    LogInfo(TRACE_USB_DEVICE, "[ INFO ] %!FUNC! USB Device SuspendAwakeState=%x", FunctionPower);

    return STATUS_SUCCESS;
}

void EvtUdecxUsbEndpointReset(
    _In_ UDECXUSBENDPOINT UdecxUsbEndpoint,
    _In_ WDFREQUEST Request
)
{
    LogInfo(TRACE_USB_DEVICE, "[ INFO ] %!FUNC! Usb endpoint reset");
    UNREFERENCED_PARAMETER(UdecxUsbEndpoint);
    UNREFERENCED_PARAMETER(Request);
}