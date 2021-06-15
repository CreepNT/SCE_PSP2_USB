#include "internal.h"
#include "driver.tmh"

HRESULT CSCEUsbDriver::CreateInstance(_In_ PCCustomTracer Tracer, _Out_ PCSCEUsbDriver *Driver) {
    PCSCEUsbDriver driver = new CSCEUsbDriver(Tracer);
    if (NULL == driver) {
        return E_OUTOFMEMORY;
    }

    HRESULT hr = driver->Initialize();
    if (SUCCEEDED(hr)) {
        *Driver = driver;
    }
    else {
        // Release the reference on the driver object to get it to delete itself.
        driver->Release();
    }

    return hr;
}

HRESULT CSCEUsbDriver::Initialize(VOID) {
    return S_OK;
}

HRESULT CSCEUsbDriver::QueryInterface(_In_ REFIID InterfaceId, _Out_ PVOID *Interface) {
    if (IsEqualIID(InterfaceId, __uuidof(IDriverEntry)))
    {
        *Interface = QueryIDriverEntry();
        return S_OK;
    }
    else return CUnknown::QueryInterface(InterfaceId, Interface);
}

HRESULT CSCEUsbDriver::OnDeviceAdd(_In_ IWDFDriver *FxWdfDriver, _In_ IWDFDeviceInitialize *FxDeviceInit) {
    Trace(TRACE_LEVEL_INFORMATION, "OnDeviceAdd()");
    m_Tracer->Log("OnDevAdd()");

    //
    // Create a new instance of our device callback object 
    //
    PCSCEUsbDevice device = NULL;
    HRESULT hr = CSCEUsbDevice::CreateInstance(FxWdfDriver, FxDeviceInit, m_Tracer, &device);

    
    //
    // If that succeeded then call the device's construct method.  This 
    // allows the device to create any queues or other structures that it
    // needs now that the corresponding fx device object has been created.
    //

    if (SUCCEEDED(hr)) 
    {
        hr = device->Configure();
    }

    // 
    // Release the reference on the device callback object now that it's been
    // associated with an fx device object.
    //

    if (NULL != device)
    {
        device->Release();
    }

    return hr;
}
