
#include "public.h"
#include "internal.h"
#include "ioqueue.tmh"

CIoQueueMgr::CIoQueueMgr(_In_ PCSCEUsbDevice Device) :
    CQueue(Device), unk58(TRUE), unk5C(0), numReadOps(0), numReadBytes(0),
    numWriteOps(0), numWrittenBytes(0) {
    m_Tracer = Device->QueryTracer(); //This adds a reference for us
}

HRESULT STDMETHODCALLTYPE CIoQueueMgr::QueryInterface(_In_ REFIID InterfaceId, _Out_ PVOID* Object) {
    HRESULT hr;
    if (IsEqualIID(InterfaceId, __uuidof(IQueueCallbackCreate))) {
        hr = S_OK;
        *Object = QueryIQueueCallbackCreate();
    }
    else if (IsEqualIID(InterfaceId, __uuidof(IQueueCallbackIoStop))) {
        hr = S_OK;
        *Object = QueryIQueueCallbackIoStop();
    }
    else if (IsEqualIID(InterfaceId, __uuidof(IQueueCallbackRead))) {
        hr = S_OK;
        *Object = QueryIQueueCallbackRead();
    }
    else if (IsEqualIID(InterfaceId, __uuidof(IQueueCallbackWrite))) {
        hr = S_OK;
        *Object = QueryIQueueCallbackWrite();
    }
    else if (IsEqualIID(InterfaceId, __uuidof(IQueueCallbackDeviceIoControl)))
    {
        hr = S_OK;
        *Object = QueryIQueueCallbackDeviceIoControl();
    }
    else if (IsEqualIID(InterfaceId, __uuidof(IRequestCallbackRequestCompletion))) {
        hr = S_OK;
        *Object = QueryIRequestCallbackRequestCompletion();
    }
    else
    {
        hr = CQueue::QueryInterface(InterfaceId, Object);
    }

    return hr;
}

HRESULT CIoQueueMgr::CreateInstance(_In_ PCSCEUsbDevice Device, _Out_ PCIoQueueMgr* Queue) {
    PCIoQueueMgr queue = NULL;
    HRESULT hr = S_OK;

    queue = new CIoQueueMgr(Device);

    if (NULL == queue)
    {
        hr = E_OUTOFMEMORY;
    }

    //
    // Call the queue callback object to initialize itself.  This will create 
    // its partner queue framework object.
    //

    if (SUCCEEDED(hr))
    {
        hr = queue->Initialize();
    }

    if (SUCCEEDED(hr))
    {
        *Queue = queue;
    }
    else
    {
        SAFE_RELEASE(queue);
    }

    return hr;
}

HRESULT CIoQueueMgr::Initialize(VOID) {
    constexpr auto DispatchType = WdfIoQueueDispatchParallel;
    constexpr bool IsDefault = true;
    constexpr bool PowerManaged = true;

    return __super::Initialize(DispatchType, IsDefault, PowerManaged);
}

void CIoQueueMgr::ForwardFormattedRequest(_In_ IWDFIoRequest* pRequest, _In_ IWDFIoTarget* pIoTarget) {
    //Set the completion callback
    IRequestCallbackRequestCompletion* pCompletionCallback = NULL;
    HRESULT hrQI = this->QueryInterface(IID_PPV_ARGS(&pCompletionCallback));
    WUDF_DRIVER_ASSERT(SUCCEEDED(hrQI) && (NULL != pCompletionCallback));

    pRequest->SetCompletionCallback(pCompletionCallback, NULL);
    pCompletionCallback->Release();
    pCompletionCallback = NULL;

    //Send down the request
    HRESULT hrSend = pRequest->Send(pIoTarget, 0, 0);

    if (FAILED(hrSend)) {
        pRequest->CompleteWithInformation(hrSend, 0);
    }
}

void STDMETHODCALLTYPE CIoQueueMgr::OnCreateFile(
    _In_  IWDFIoQueue* pWdfQueue,
    _In_  IWDFIoRequest* pWdfRequest,
    _In_  IWDFFile* pWdfFileObject) {
    UNREFERENCED_PARAMETER(pWdfQueue);
    UNREFERENCED_PARAMETER(pWdfRequest);
    UNREFERENCED_PARAMETER(pWdfFileObject);

    m_Device->GetUsbInputPipe()->AddRef();
    m_Device->GetUsbOutputPipe()->AddRef();
}

void STDMETHODCALLTYPE CIoQueueMgr::OnIoStop(
    _In_  IWDFIoQueue* pWdfQueue,
    _In_  IWDFIoRequest* pWdfRequest,
    _In_  ULONG ActionFlags) {

    UNREFERENCED_PARAMETER(pWdfQueue);
    UNREFERENCED_PARAMETER(ActionFlags);
    Trace(TRACE_LEVEL_INFORMATION, "OnIoStop()");
    m_Tracer->Log("OnIoStop()");
    pWdfRequest->CancelSentRequest();
}

void STDMETHODCALLTYPE CIoQueueMgr::OnRead(
    _In_  IWDFIoQueue* pWdfQueue,
    _In_  IWDFIoRequest* pWdfRequest,
    _In_  SIZE_T NumOfBytesToRead) {

    UNREFERENCED_PARAMETER(pWdfQueue);
    UNREFERENCED_PARAMETER(NumOfBytesToRead);

    if (!unk58) {
        pWdfRequest->Complete(ERROR_READ_FAULT);
        return;
    }

    IWDFUsbTargetPipe* pInputPipe = m_Device->GetUsbInputPipe();

    IWDFMemory* outMemory = NULL;
    pWdfRequest->GetOutputMemory(&outMemory);
    
    HRESULT hr = pInputPipe->FormatRequestForRead(pWdfRequest, NULL, outMemory, NULL, NULL);
    if (FAILED(hr)) {
        pWdfRequest->Complete(hr);
        return;
    }
    ForwardFormattedRequest(pWdfRequest, pInputPipe);
    SAFE_RELEASE(outMemory);
}

void STDMETHODCALLTYPE CIoQueueMgr::OnWrite(
    _In_  IWDFIoQueue* pWdfQueue,
    _In_  IWDFIoRequest* pWdfRequest,
    _In_  SIZE_T NumOfBytesToWrite) {

    UNREFERENCED_PARAMETER(pWdfQueue);
    UNREFERENCED_PARAMETER(NumOfBytesToWrite);

    IWDFUsbTargetPipe* pOutputPipe = m_Device->GetUsbOutputPipe();

    IWDFMemory* inMemory = NULL;
    pWdfRequest->GetInputMemory(&inMemory);

    HRESULT hr = pOutputPipe->FormatRequestForWrite(pWdfRequest, NULL, inMemory, NULL, NULL);
    if (FAILED(hr)) {
        pWdfRequest->Complete(hr);
    }
    else {
        ForwardFormattedRequest(pWdfRequest, pOutputPipe);
    }
    SAFE_RELEASE(inMemory);
}


void STDMETHODCALLTYPE CIoQueueMgr::OnDeviceIoControl(
    _In_  IWDFIoQueue* pWdfQueue,
    _In_  IWDFIoRequest* pWdfRequest,
    _In_  ULONG ControlCode,
    _In_  SIZE_T InputBufferSizeInBytes,
    _In_  SIZE_T OutputBufferSizeInBytes) {

    UNREFERENCED_PARAMETER(pWdfQueue);
    UNREFERENCED_PARAMETER(InputBufferSizeInBytes);
    UNREFERENCED_PARAMETER(OutputBufferSizeInBytes);

    //Prepare output memory for all Ioctls
    IWDFMemory* outMemory = NULL;
    void* dataBuf = NULL;
    SIZE_T dataBufSize = 0;
    pWdfRequest->GetOutputMemory(&outMemory);
    if (outMemory == NULL) 
        pWdfRequest->CompleteWithInformation(E_NOTIMPL, 0);

    dataBuf = outMemory->GetDataBuffer(&dataBufSize);
    if (dataBuf == NULL)
        pWdfRequest->CompleteWithInformation(E_NOTIMPL, 0);


    HRESULT CompletionStatus = E_NOTIMPL; //This is the generic error the driver returns on all errors (except wrong ControlCode)
    SIZE_T OutputSize = 0; //On error, the output size should be 0. By using it as default, we only change it in success pathes and remove redundant code.

    switch (ControlCode) {
    case IOCTL_DECI4P_GET_IOQUEUE_STATUS:
        if (dataBufSize > 0) {
            UCHAR outputData = 0;
            outputData |= (m_Device->GetProductId() != 0x432U) ? 0x40U : 0U;
            outputData |= (unk58) ? 0x80U : 0U;
            CompletionStatus = outMemory->CopyFromBuffer(NULL, &outputData, sizeof(outputData));
            if (SUCCEEDED(CompletionStatus)) {
                OutputSize = 1;
            }

            if (!unk58 && unk5C > 0) {
                unk5C--;
                if (unk5C == 0) unk58 = TRUE;
            }
            break;
        }
        else break; //Return E_NOTIMPL

    case IOCTL_DECI4P_GET_TRACING_INFO:
        if (dataBufSize >= EXPOSED_DATA_HEADER_SIZE) {
            CompletionStatus = outMemory->CopyFromBuffer(NULL, &m_Tracer->expData, EXPOSED_DATA_HEADER_SIZE);
            if (SUCCEEDED(CompletionStatus)) {
                if (dataBufSize >= EXPOSED_DATA_SIZE) {
                    CompletionStatus = outMemory->CopyFromBuffer(NULL, &(m_Tracer->expData.buf), EXPOSED_TRACE_BUFFER_SIZE);
                    if (SUCCEEDED(CompletionStatus)) OutputSize = EXPOSED_DATA_SIZE;
                }
                else OutputSize = EXPOSED_DATA_HEADER_SIZE;
                break; //Return the CopyFromBuffer error/success
            }
            else break; //Return the CopyFromBuffer error, with size 0.
        }
        else break; //Return E_NOTIMPL

    case IOCTL_DECI4P_GET_DRIVER_STATS:
        if (dataBufSize > 0) {
            DriverStats drvStats = { 0 };
            drvStats.size = sizeof(drvStats); static_assert(sizeof(drvStats) == 0x2C, "DriverStats structure size is invalid.");
            drvStats.clockSpeed = 0x1800000;
            drvStats.notAtHighSpeed = (m_Device->GetSpeed() != HighSpeed);
            drvStats.numReadOps = this->numReadOps;
            drvStats.numReadBytes = this->numReadBytes;
            drvStats.numWriteOps = this->numWriteOps;
            drvStats.numWrittenBytes = this->numWrittenBytes;
            SIZE_T copySize = (dataBufSize > sizeof(drvStats)) ? sizeof(drvStats) : dataBufSize;
            CompletionStatus = outMemory->CopyFromBuffer(NULL, &drvStats, copySize);
            if (SUCCEEDED(CompletionStatus)) {
                OutputSize = copySize;
            }
        }
        else break; //Return E_NOTIMPL

    case IOCTL_DECI4P_RESET_INPUT_PIPE:
        CompletionStatus = m_Device->GetUsbInputPipe()->Reset();
        break;

    case IOCTL_DECI4P_RESET_OUTPUT_PIPE:
        CompletionStatus = m_Device->GetUsbOutputPipe()->Reset();
        break;

    default:
        Trace(TRACE_LEVEL_ERROR, "%!FUNC! : Unknown Ioctl control code 0x%lX requested.", ControlCode);
        CompletionStatus = HRESULT_FROM_WIN32(ERROR_INVALID_FUNCTION);
        break;
    }

    SAFE_RELEASE(outMemory);
    pWdfRequest->CompleteWithInformation(CompletionStatus, OutputSize);
    return;
}


void STDMETHODCALLTYPE CIoQueueMgr::OnCompletion(
    _In_  IWDFIoRequest* pWdfRequest,
    _In_  IWDFIoTarget* pIoTarget,
    _In_  IWDFRequestCompletionParams* pParams,
    _In_  void* pContext) {

    UNREFERENCED_PARAMETER(pIoTarget);
    UNREFERENCED_PARAMETER(pContext);

    HRESULT CompletionStatus = pParams->GetCompletionStatus();
    SIZE_T Information = pParams->GetInformation();
    if (SUCCEEDED(CompletionStatus)) {
        WDF_REQUEST_TYPE reqType = pWdfRequest->GetType();
        CHAR tmpBuf[0x10] = { 0 };
        if (reqType == WdfRequestRead) {
            numReadOps++;
            numReadBytes += Information;
            Trace(TRACE_LEVEL_INFORMATION, "<- READ %05Id bytes", Information);
            sprintf_s<sizeof(tmpBuf)>(tmpBuf, "<- %05Idbytes", Information);
        }
        else if (reqType == WdfRequestWrite) {
            numWriteOps++;
            numWrittenBytes += Information;
            Trace(TRACE_LEVEL_INFORMATION, "-> WRITE %05Id bytes", Information);
            sprintf_s<sizeof(tmpBuf)>(tmpBuf, "-> %05Idbytes", Information);
        }
        m_Tracer->Log(tmpBuf);
    }
    else {
        Trace(TRACE_LEVEL_ERROR, "%!FUNC! : %!HRESULT! on request.", CompletionStatus);
    }
    pWdfRequest->CompleteWithInformation(CompletionStatus, Information);
}