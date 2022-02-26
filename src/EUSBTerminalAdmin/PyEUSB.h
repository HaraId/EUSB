#pragma once

#include <windows.h>


#ifdef UNICODE
#define MAKEINTRESOURCEA_T(a, u) MAKEINTRESOURCEA(u)
#else
#define MAKEINTRESOURCEA_T(a, u) MAKEINTRESOURCEA(a)
#endif


#define LogError(desc) \
    std::cout << "EUSB [ ERROR ] " << __func__ << ": " desc << std::endl

/*
#define EXPORT_FUNCTION_FROM_EXTERNAL_LIB(lib, funcName, distFuncName)                                  \
    if ( (distFuncName = (funcName)GetProcAddress(                                                      \
        lib,                                                                                            \
        #funcName                                                                                       \
        )) == nullptr)                                                                                  \
    {                                                                                                   \
        LogError(#funcName " not found!");                                                              \
        FreeLibrary(lib);                                                                               \
        return PyLong_FromLong(1);                                                                      \
    }*/


HINSTANCE hInstShell32  = nullptr;
HINSTANCE hInstCfgmgr32 = nullptr;

typedef BOOL(WINAPI* LPFN_GUIDFromString)(
    LPCTSTR, 
    LPGUID
);
LPFN_GUIDFromString pGUIDFromString = nullptr;

typedef CONFIGRET (WINAPI* __CM_Get_Device_Interface_ListA)
(
    _In_           LPGUID      InterfaceClassGuid,
    _In_opt_       DEVINSTID_A pDeviceID,
    _Out_          PZZSTR      Buffer,
    _In_           ULONG       BufferLen,
    _In_           ULONG       ulFlags
);
__CM_Get_Device_Interface_ListA pCM_Get_Device_Interface_ListA = nullptr;

typedef CONFIGRET (WINAPI* __CM_Get_Device_Interface_List_SizeA)
(
    _Out_          PULONG      pulLen,
    _In_           LPGUID      InterfaceClassGuid,
    _In_opt_       DEVINSTID_A pDeviceID,
    _In_           ULONG       ulFlags
);
__CM_Get_Device_Interface_List_SizeA pCM_Get_Device_Interface_List_SizeA = nullptr;

/*
 *
 * python setup.py install
 * python -i testmodule.py
 *
 * */
