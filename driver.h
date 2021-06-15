#pragma once

#include "customTracer.h"

class CSCEUsbDriver : public CUnknown, public IDriverEntry {
//Private data
private:
    PCCustomTracer m_Tracer;

// Private methods
private:
    CSCEUsbDriver(PCCustomTracer Tracer) : m_Tracer(Tracer) {
        Tracer->AddRef();
    }

    virtual ~CSCEUsbDriver() {
        m_Tracer->Release();
    }

    // Returns a referenced pointer to the IDriverEntry interface.
    IDriverEntry* QueryIDriverEntry(VOID) {
        AddRef();
        return static_cast<IDriverEntry*>(this);
    }

    HRESULT Initialize(VOID);

// Public methods
public:
    // The factory method used to create an instance of this driver.
    static HRESULT CreateInstance(_In_ PCCustomTracer Tracer, _Out_ PCSCEUsbDriver *Driver);

// COM methods
public:
    // IDriverEntry methods
    virtual HRESULT STDMETHODCALLTYPE OnInitialize(_In_ IWDFDriver *FxWdfDriver) {
        UNREFERENCED_PARAMETER( FxWdfDriver );
        m_Tracer->Log("OnInit()");
        return S_OK;
    }

    virtual HRESULT STDMETHODCALLTYPE OnDeviceAdd(_In_ IWDFDriver *FxWdfDriver, _In_ IWDFDeviceInitialize *FxDeviceInit);
    
    virtual VOID STDMETHODCALLTYPE OnDeinitialize(_In_ IWDFDriver *FxWdfDriver) {
        UNREFERENCED_PARAMETER( FxWdfDriver );
        m_Tracer->Log("OnDeinit()");
        return;
    }

    //
    // IUnknown methods.
    //
    // We have to implement basic ones here that redirect to the 
    // base class because of the multiple inheritance.
    //
    virtual ULONG STDMETHODCALLTYPE AddRef(VOID) {
        return __super::AddRef();
    }
    
    _At_(this, __drv_freesMem(object))
    virtual ULONG STDMETHODCALLTYPE Release(VOID) {
        return __super::Release();
    }

    virtual HRESULT STDMETHODCALLTYPE QueryInterface(_In_ REFIID InterfaceId,_Out_ PVOID *Object);
};
