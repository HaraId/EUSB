#pragma once
/*++

Module Name:

    public.h

Abstract:

    This module contains the common declarations shared by driver
    and user applications.

Environment:

    user and kernel

--*/

#define EUSB_BUFF_TAG 'EUSB'

//
// Define an Interface Guid so that apps can find the device and talk to it.
//
#ifdef DEFINE_GUID

DEFINE_GUID (GUID_DEVINTERFACE_EUSB,
    0x752808f4,0xa9e9,0x42f4,0x81,0x15,0x64,0x40,0xc9,0x87,0x13,0xb1);
// {752808f4-a9e9-42f4-8115-6440c98713b1}

#endif
