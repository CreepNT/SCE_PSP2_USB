#include "internal.h"
#include "dllsup.tmh"

const GUID CLSID_SonyDriverCoClass = SCEUSB_DRIVER_CLASSID;

BOOL WINAPI DllMain(HINSTANCE ModuleHandle, DWORD Reason, PVOID /* Reserved */){

    UNREFERENCED_PARAMETER( ModuleHandle );

    if (DLL_PROCESS_ATTACH == Reason)
    {
        //
        // Initialize tracing.
        //

        WPP_INIT_TRACING(SCEUSB_TRACING_ID);

    }
    else if (DLL_PROCESS_DETACH == Reason)
    {
        //
        // Cleanup tracing.
        //

        WPP_CLEANUP();
    }

    return TRUE;
}

HRESULT STDAPICALLTYPE DllGetClassObject(_In_ REFCLSID ClassId, _In_ REFIID InterfaceId, _Outptr_ LPVOID *Interface) {
    PCClassFactory factory = NULL;
    PCCustomTracer tracer = NULL;

    HRESULT hr = S_OK;

    *Interface = NULL;

    //
    // If the CLSID doesn't match that of our "coclass" (defined in the IDL 
    // file) then we can't create the object the caller wants.  This may 
    // indicate that the COM registration is incorrect, and another CLSID 
    // is referencing this drvier.
    //

    if (IsEqualCLSID(ClassId, CLSID_SonyDriverCoClass) == false)
    {
        Trace(
            TRACE_LEVEL_ERROR,
            L"ERROR: Called to create instance of unrecognized class (%!GUID!)",
            &ClassId
            );

        return CLASS_E_CLASSNOTAVAILABLE;
    }



    tracer = new CCustomTracer();
    if (NULL == tracer) {
        hr = E_OUTOFMEMORY;
    }

    if (SUCCEEDED(hr)) {
        factory = new CClassFactory(tracer);
        if (NULL == factory) {
            hr = E_OUTOFMEMORY;
        }
        //This destroys the tracer if factory creation failed, else the factory maintained a reference and it'll stay active.
        tracer->Release();
    }

    // 
    // Query the object we created for the interface the caller wants.  After
    // that we release the object.  This will drive the reference count to 
    // 1 (if the QI succeeded and referenced the object) or 0 (if the QI failed).
    // In the later case the object is automatically deleted.
    //

    if (SUCCEEDED(hr)) 
    {
        hr = factory->QueryInterface(InterfaceId, Interface);
        factory->Release();
    }

    return hr;
}
