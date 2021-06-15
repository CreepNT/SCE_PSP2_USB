#include "internal.h"
#include "queue.h"

CQueue::CQueue(_In_ PCSCEUsbDevice Device) : m_FxQueue(NULL), m_Device(Device) {}

HRESULT CQueue::Initialize(_In_ WDF_IO_QUEUE_DISPATCH_TYPE DispatchType, _In_ bool Default, _In_ bool PowerManaged) {
    IWDFIoQueue* fxQueue;
    HRESULT hr;

    //
    // Create the I/O Queue object.
    //
    {
        IUnknown* callback = QueryIUnknown();

        hr = m_Device->GetFxDevice()->CreateIoQueue(
            callback,
            Default,
            DispatchType,
            PowerManaged,
            FALSE,
            &fxQueue
        );
        callback->Release();
    }

    if (SUCCEEDED(hr))
    {
        m_FxQueue = fxQueue;

        //
        // Release the creation reference on the queue.  This object will be 
        // destroyed before the queue so we don't need to have a reference out 
        // on it.
        //

        fxQueue->Release();
    }

    return hr;
}
