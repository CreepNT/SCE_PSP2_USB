#pragma once

#include <initguid.h>

#define WINDOWS_LEAN_AND_MEAN 1
#include <Windows.h>
#include <winioctl.h>

// {5e2fe4e5-107d-4c43-9708-8bde6508cec8}
DEFINE_GUID(GUID_DEVINTERFACE_PSVDEVKIT,
    0x5E2FE4E5, 0x107D, 0x4C43, 0x97, 0x08, 0x8B, 0xDE, 0x65, 0x08, 0xCE, 0xC8);

#pragma pack(push, 1)
typedef struct DriverStats {
    UINT32  size;           //Set to 0x2C (sizeof(this))
    UINT32  clockSpeed;     //Not sure - could be clock speed used for comunication with devkit (hardcoded to 0x1800000/25165824)
    BOOL    notAtHighSpeed; //FALSE if the USB link runs at High Speed, TRUE otherwise
    //Note : driver seems to assume that the peripheral can only run in High Speed or Full Speed mode,
    //so this can be interpreted as "runningAtFullSpeed"

    ULONG64 numReadOps;     //Number of read operations served since the (driver started/device was connected)
    ULONG64 numReadBytes;   //Number of bytes read since the (...)
    ULONG64 numWriteOps;    //Number of write operations since the (...)
    ULONG64 numWrittenBytes;//Number of bytes written since the (...)
} DriverStats;
#pragma pack(pop)

/* Get IoQueue Status (0x222000)
    Takes no input
    Outputs 1 byte  -- bit 0x40 set if the device PID doesn't match expected IDPRODUCT_DEVKIT (0x0432)
                    -- bit 0x80 set if IoQueue's unk58 is set

    This also decreases unk5C if unk58 is not set, and sets unk58 if unk5C reaches zero.
*/
#define IOCTL_DECI4P_GET_IOQUEUE_STATUS CTL_CODE(FILE_DEVICE_UNKNOWN, 0x800, METHOD_BUFFERED, FILE_ANY_ACCESS)

/* Get Tracing Info (0x222004)
    Takes no input
    Outputs 20 / 20020 bytes depending on provided output size
        --first 20 bytes are ???
        --next 20000 are trace buffer (char[])
*/
#define IOCTL_DECI4P_GET_TRACING_INFO CTL_CODE(FILE_DEVICE_UNKNOWN, 0x801, METHOD_BUFFERED, FILE_ANY_ACCESS)

/* Get Driver Stats (0x222008)
   Takes no input
   Outputs up to 0x2C bytes -- full data composes a DriverStats struct
  
   If output buffer size is less than 0x2C bytes, the output data is the result of :
    memcpy(outputBuffer, localDriversState, sizeof(outputBuffer));
*/
#define IOCTL_DECI4P_GET_DRIVER_STATS CTL_CODE(FILE_DEVICE_UNKNOWN, 0x802, METHOD_BUFFERED, FILE_ANY_ACCESS)

/* Reset Input Pipe (0x222020)
    Takes no input
    Returns no output (besides return code)

    This calls CIoQueue.m_Device->GetUsbInputPipe()->Reset()
*/
#define IOCTL_DECI4P_RESET_INPUT_PIPE CTL_CODE(FILE_DEVICE_UNKNOWN, 0x808, METHOD_BUFFERED, FILE_ANY_ACCESS)

/* Reset Input Pipe (0x222024)
    Takes no input
    Returns no output (besides return code)

    This calls CIoQueue.m_Device->GetUsbOutputPipe()->Reset()
*/
#define IOCTL_DECI4P_RESET_OUTPUT_PIPE CTL_CODE(FILE_DEVICE_UNKNOWN, 0x809, METHOD_BUFFERED, FILE_ANY_ACCESS)