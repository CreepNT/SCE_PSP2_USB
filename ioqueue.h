#pragma once

#include "internal.h"

typedef class CIoQueueMgr *PCIoQueueMgr;

class CIoQueueMgr : 
    public IQueueCallbackCreate, public IQueueCallbackIoStop, public IQueueCallbackRead, 
    public IQueueCallbackWrite, public IQueueCallbackDeviceIoControl, public IRequestCallbackRequestCompletion, 
    public CQueue {

//Private data
private:
    PCCustomTracer m_Tracer;

//Public data
public:
    BOOL unk58;
    INT unk5C; //Some counter ?
    ULONG64 numReadOps;
    ULONG64 numReadBytes;
    ULONG64 numWriteOps;
    ULONG64 numWrittenBytes;

//Protected methods
protected:
    HRESULT Initialize(VOID);

    void ForwardFormattedRequest(_In_ IWDFIoRequest* pRequest, _In_ IWDFIoTarget* pIoTarget);

//Public methods
public:
    CIoQueueMgr(_In_ PCSCEUsbDevice Device);

    virtual ~CIoQueueMgr(VOID) {
        m_Tracer->Release();
    };

    static HRESULT CreateInstance(_In_ PCSCEUsbDevice Device, _Out_ PCIoQueueMgr* Queue);

    HRESULT Configure(VOID) {
        return S_OK;
    }

    IQueueCallbackCreate* QueryIQueueCallbackCreate(VOID) {
        AddRef();
        return static_cast<IQueueCallbackCreate*>(this);
    }

    IQueueCallbackIoStop* QueryIQueueCallbackIoStop(VOID) {
        AddRef();
        return static_cast<IQueueCallbackIoStop*>(this);
    }
    
    IQueueCallbackRead* QueryIQueueCallbackRead(VOID) {
        AddRef();
        return static_cast<IQueueCallbackRead*>(this);
    }
    
    IQueueCallbackWrite* QueryIQueueCallbackWrite(VOID) {
        AddRef();
        return static_cast<IQueueCallbackWrite*>(this);
    }

    IQueueCallbackDeviceIoControl* QueryIQueueCallbackDeviceIoControl(VOID) {
        AddRef();
        return static_cast<IQueueCallbackDeviceIoControl*>(this);
    }

    IRequestCallbackRequestCompletion* QueryIRequestCallbackRequestCompletion(VOID) {
        AddRef();
        return static_cast<IRequestCallbackRequestCompletion*>(this);
    }

//COM methods
public:
    // IUnknown
    virtual ULONG STDMETHODCALLTYPE AddRef(VOID) { return CUnknown::AddRef(); }

    _At_(this, __drv_freesMem(object))
    virtual ULONG STDMETHODCALLTYPE Release(VOID) { return CUnknown::Release(); }

    virtual HRESULT STDMETHODCALLTYPE QueryInterface(_In_ REFIID InterfaceId, _Out_ PVOID* Object);


    //IQueueCallbackCreate
    virtual void STDMETHODCALLTYPE OnCreateFile(
        _In_  IWDFIoQueue* pWdfQueue,
        _In_  IWDFIoRequest* pWDFRequest,
        _In_  IWDFFile* pWdfFileObject);

    //IQueueCallbackIoStop
    virtual void STDMETHODCALLTYPE OnIoStop(
        _In_  IWDFIoQueue* pWdfQueue,
        _In_  IWDFIoRequest* pWdfRequest,
        _In_  ULONG ActionFlags);

    //IQueueCallbackRead
    virtual void STDMETHODCALLTYPE OnRead(
        _In_  IWDFIoQueue* pWdfQueue,
        _In_  IWDFIoRequest* pWdfRequest,
        _In_  SIZE_T NumOfBytesToRead);

    //IQueueCallbackWrite
    virtual void STDMETHODCALLTYPE OnWrite(
        _In_  IWDFIoQueue* pWdfQueue,
        _In_  IWDFIoRequest* pWdfRequest,
        _In_  SIZE_T NumOfBytesToWrite);

    //IQueueCallbackDeviceIoControl
    virtual void STDMETHODCALLTYPE OnDeviceIoControl(
        _In_  IWDFIoQueue* pWdfQueue,
        _In_  IWDFIoRequest* pWdfRequest,
        _In_  ULONG ControlCode,
        _In_  SIZE_T InputBufferSizeInBytes,
        _In_  SIZE_T OutputBufferSizeInBytes);

    //IRequestCallbackRequestCompletion
    virtual void STDMETHODCALLTYPE OnCompletion(
        _In_  IWDFIoRequest* pWdfRequest,
        _In_  IWDFIoTarget* pIoTarget,
        _In_  IWDFRequestCompletionParams* pParams,
        _In_  void* pContext);
};