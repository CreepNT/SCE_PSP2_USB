#pragma once

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(x) (sizeof(x) / sizeof(x[0]))
#endif

//
// Include the WUDF DDI 
//

#include "wudfddi.h"

//
// Include the WUDF USB DDI
//

#include "wudfusb.h"

//
// Use specstrings for in/out annotation of function parameters.
//

#include "specstrings.h"

//
// Forward definitions of classes in the other header files.
//

typedef class CSCEUsbDriver *PCSCEUsbDriver;
typedef class CSCEUsbDevice *PCSCEUsbDevice;

//
// Driver specific #defines
//

#define SCEUSB_TRACING_ID L"Sony\\VitaDevkitUsb"

//GUID : {DC10F744-087C-4657-84BE-FA870C94376F}
#define SCEUSB_DRIVER_CLASSID { 0xDC10F744, 0x087C, 0x4657, { 0x84, 0xBE, 0xFA, 0x87, 0x0C, 0x94, 0x37, 0x6F } }

#define IDPRODUCT_DEVKIT 0x432
#define BCDDEVICE_NEWREV 0x220 //Checked in OnD0Entry, if bcdDevice < this then additional processing is required.

//
// Define the tracing flags.
//
// TODO: Choose a different trace control GUID
//

#define WPP_CONTROL_GUIDS                                                   \
    WPP_DEFINE_CONTROL_GUID(                                                \
        SceUsbDriverTraceControl, (e7541cdd,30e8,4b50,aeb0,51927330ae64),   \
                                                                            \
        WPP_DEFINE_BIT(MYDRIVER_ALL_INFO)                                   \
        )

#define WPP_FLAG_LEVEL_LOGGER(flag, level)                                  \
    WPP_LEVEL_LOGGER(flag)

#define WPP_FLAG_LEVEL_ENABLED(flag, level)                                 \
    (WPP_LEVEL_ENABLED(flag) &&                                             \
     WPP_CONTROL(WPP_BIT_ ## flag).Level >= level)

//
// begin_wpp config
// FUNC Trace{FLAG=MYDRIVER_ALL_INFO}(LEVEL, MSG, ...);
// WPP_FLAGS(-public:Trace)
// end_wpp
//

__forceinline 
#ifdef _PREFAST_
__declspec(noreturn)
#endif
VOID WdfTestNoReturn(VOID) {
    // do nothing.
}

#define WUDF_DRIVER_ASSERT(p)  \
{                                     \
    if ( !(p) )                       \
    {                                 \
        DebugBreak();                 \
        WdfTestNoReturn();            \
    }                                 \
}

#define SAFE_RELEASE(x) if (x != NULL) {x->Release(); x = NULL;}

//
// Include the type specific headers.
//
#include "comsup.h"
#include "driver.h"
#include "device.h"
#include "queue.h"
#include "ioqueue.h"
#include "customTracer.h"