#define _CRT_SECURE_NO_WARNINGS

#include <Python.h>
#include <driverspecs.h>
#include <iostream>
#include <windows.h>
#include <cfgmgr32.h>
#include <basetyps.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <conio.h>
#include "devioctl.h"
#include "strsafe.h"
#include "PyEUSB.h"

#include <shellapi.h>


using namespace std;

#define EUSB_BUFF_TAG 'EUSB'
#define MAX_DEVPATH_LENGTH 256

// Исключение которое мы будем бросать в случае какой-то ошибки
static PyObject* LosetupError;


static PyObject* init_eusb(PyObject* self, PyObject* args)
{
    /*
        Load additional functons from shell32.dll

        Funcs:
            1) GuidFromString (as pGuidFromString) - for convert strings guid format in "GUID" struct.
    */
    hInstShell32 = LoadLibrary(TEXT("shell32.dll"));
    if (!hInstShell32)
    {
        LogError("Shell32 lib not loaded!");
        return PyLong_FromLong(1);
    }

    pGUIDFromString = (LPFN_GUIDFromString)GetProcAddress(hInstShell32, MAKEINTRESOURCEA_T(703, 704));
    if (pGUIDFromString == nullptr)
    {
        LogError("pGUIDFromString not found!");
        FreeLibrary(hInstShell32);
        return PyLong_FromLong(1);
    }

    /*
        Load additional functions from CfgMgr32.dll

        Funcs:
            1) CM_Get_Device_Interface_List_SizeA (as pCM_Get_Device_Interface_List_SizeA)
            2) CM_Get_Device_Interface_ListA (as pCM_Get_Device_Interface_ListA)
    */
    hInstCfgmgr32 = LoadLibrary(TEXT("CfgMgr32.dll"));
    if (!hInstCfgmgr32)
    {
        LogError("hInstCfgmgr32 lib not loaded!");
        return PyLong_FromLong(2);
    }

    pCM_Get_Device_Interface_ListA = (__CM_Get_Device_Interface_ListA)GetProcAddress(
        hInstCfgmgr32,
        "CM_Get_Device_Interface_ListA"
    );
    if (pCM_Get_Device_Interface_ListA == nullptr)
    {
        LogError("pCM_Get_Device_Interface_ListA not found!");
        FreeLibrary(hInstCfgmgr32);
        return PyLong_FromLong(2);
    }

    pCM_Get_Device_Interface_List_SizeA = (__CM_Get_Device_Interface_List_SizeA)GetProcAddress(
        hInstCfgmgr32,
        "CM_Get_Device_Interface_List_SizeA"
    );
    if (pCM_Get_Device_Interface_List_SizeA == nullptr)
    {
        LogError("CM_Get_Device_Interface_List_SizeA not found!");
        FreeLibrary(hInstCfgmgr32);
        return PyLong_FromLong(2);
    }

    return PyLong_FromLong(0);
}

static PyObject* free_eusb(PyObject* self, PyObject* args)
{
    if (hInstShell32)
        FreeLibrary(hInstShell32);
    if (hInstCfgmgr32)
        FreeLibrary(hInstCfgmgr32);

    return PyLong_FromLong(0);
}

//
// Arguments:
// 1) guid in string format: {00000000-0000-0000-0000-000000000000}
// example: "{752808f4-a9e9-42f4-8115-6440c98713b1}"
static PyObject* get_device_by_guid(PyObject* self, PyObject* args)
{
    CONFIGRET cr = CR_SUCCESS;
    PWSTR deviceInterfaceList = nullptr;
    ULONG deviceInterfaceListLength = 0;
    PWSTR nextInterface;
    HRESULT hr = E_FAIL;
    BOOL bRet = TRUE;

    CHAR DevicePath[256] = { 0 };


    char* strGuid;
    GUID InterfaceGuid;
    if (!PyArg_ParseTuple(args, "s", &strGuid)) {
        LogError("can not parse arguments...");
        return nullptr;
    }
    printf("Guid : %s\n", strGuid);

    if (!pGUIDFromString((LPCTSTR)strGuid, &InterfaceGuid))
    {
        LogError("can not convert str guid to GUID");
        return nullptr;
    }

    cr = pCM_Get_Device_Interface_List_SizeA(
        &deviceInterfaceListLength,
        &InterfaceGuid,              // _In_  LPGUID InterfaceGuid,
        nullptr,
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
    if (deviceInterfaceList == nullptr) {
        bRet = FALSE;
        printf("Error  allocating memory for device interface list.\n");
        goto clean0;
    }
    ZeroMemory(deviceInterfaceList, deviceInterfaceListLength * sizeof(WCHAR));

    cr = pCM_Get_Device_Interface_ListA(
        &InterfaceGuid,
        nullptr,
        (PZZSTR)deviceInterfaceList,
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

    hr = StringCchCopyA((STRSAFE_LPSTR)DevicePath, 256, (STRSAFE_LPSTR)deviceInterfaceList);
    if (FAILED(hr)) {
        bRet = FALSE;
        printf("Error: StringCchCopy failed with HRESULT 0x%lx", hr);
        goto clean0;
    }

clean0:
    if (deviceInterfaceList != nullptr) {
        free(deviceInterfaceList);
    }
    if (CR_SUCCESS != cr) {
        bRet = FALSE;
    }

    return _PyUnicode_FromASCII(DevicePath, strlen(DevicePath));
}


// Таблица методов реализуемых расширением
// название, функция, параметры, описание
static PyMethodDef EUSB_Methods[] = {
    {"get_device_by_guid",  get_device_by_guid, METH_VARARGS, "Usage _eusb.get_device_by_guid(guid)."},
    {"init_eusb",  init_eusb, METH_VARARGS, "Usage _eusb.init_eusb()."},
    {"free_eusb",  free_eusb, METH_VARARGS, "Usage _eusb.free_eusb()."},
    {nullptr, nullptr, 0, nullptr}
};

static struct PyModuleDef eusb_module = {
    PyModuleDef_HEAD_INIT,
    "eusb",
    "Python interface for the fputs C library function",
    -1,
    EUSB_Methods
};

PyMODINIT_FUNC PyInit_eusb(void) {
    return PyModule_Create(&eusb_module);
}
