#pragma once

#include <ntddk.h>
#include <wdf.h>

EXTERN_C_START

typedef struct _BUFFER_CONTENT
{
    LIST_ENTRY  BufferLink;
    SIZE_T      BufferLength;
    UCHAR       BufferStart; // variable-size structure, first byte of last field
} BUFFER_CONTENT, * PBUFFER_CONTENT;

typedef struct _WRITE_BUFFER_TO_READ_REQUEST_QUEUE
{
    LIST_ENTRY WriteBufferQueue; // write data comes in, keep it here to complete a read request
    WDFQUEUE   ReadBufferQueue; // read request comes in, stays here til a matching write buffer arrives
    WDFSPINLOCK qsync;
} WRITE_BUFFER_TO_READ_REQUEST_QUEUE, * PWRITE_BUFFER_TO_READ_REQUEST_QUEUE;

NTSTATUS InitCommandPipesQueue(_In_ const WDFDEVICE* wdfDevice);

EVT_WDF_IO_QUEUE_IO_DEVICE_CONTROL ControllerEvtIoDeviceControl;
EVT_WDF_IO_QUEUE_IO_READ CommandPipeEvtRead;
EVT_WDF_IO_QUEUE_IO_WRITE CommandPipeEvtWrite;

EXTERN_C_END