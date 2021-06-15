#pragma once

#include "comsup.h"
#include "internal.h"

//
// Queue Callback Object.
//
class CQueue : public CUnknown {
protected:
    // Unreferenced pointer to the partner Fx device.
    IWDFIoQueue* m_FxQueue;

    // Unreferenced pointer to the parent device.
    PCSCEUsbDevice m_Device;

    HRESULT Initialize(_In_ WDF_IO_QUEUE_DISPATCH_TYPE DispatchType, _In_ bool Default, _In_ bool PowerManaged);

protected:

    CQueue(_In_ PCSCEUsbDevice Device);
    virtual ~CQueue(VOID) {};

    HRESULT Configure(VOID) { return S_OK; };

public:

    IWDFIoQueue* GetFxQueue(VOID) {
        return m_FxQueue;
    }


    PCSCEUsbDevice GetDevice(VOID) {
        return m_Device;
    }

    //
    // IUnknown
    //
    virtual ULONG STDMETHODCALLTYPE AddRef(VOID) { return CUnknown::AddRef(); }

    _At_(this, __drv_freesMem(object))
    virtual ULONG STDMETHODCALLTYPE Release(VOID) { return CUnknown::Release(); }

    virtual HRESULT STDMETHODCALLTYPE QueryInterface(_In_ REFIID InterfaceId, _Outptr_ PVOID* Object) {
        return CUnknown::QueryInterface(InterfaceId, Object);
    }
};