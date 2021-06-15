#include "public.h"

#include "internal.h"
#include "device.tmh"



HRESULT CSCEUsbDevice::CreateInstance(_In_ IWDFDriver *FxDriver, _In_ IWDFDeviceInitialize * FxDeviceInit, _In_ PCCustomTracer Tracer, _Out_ PCSCEUsbDevice *Device) {
    //
    // Allocate a new instance of the device class.
    //

    PCSCEUsbDevice device = new CSCEUsbDevice(Tracer);

    if (NULL == device) {
        return E_OUTOFMEMORY;
    }

    //
    // Initialize the instance.
    //

    HRESULT hr = device->Initialize(FxDriver, FxDeviceInit);

    if (SUCCEEDED(hr)) {
        *Device = device;
    } 
    else {
        device->Release();
    }

    return hr;
}

HRESULT CSCEUsbDevice::Initialize(_In_ IWDFDriver* FxDriver, _In_ IWDFDeviceInitialize* FxDeviceInit) {
    FxDeviceInit->SetLockingConstraint(WdfDeviceLevel);

    //
    // Create a new FX device object and assign the new callback object to 
    // handle any device level events that occur.
    //
    IWDFDevice* fxDevice;
    HRESULT hr;
    {
        IUnknown *unknown = this->QueryIUnknown();

        hr = FxDriver->CreateDevice(FxDeviceInit, unknown, &fxDevice);

        unknown->Release();
    }

    if (SUCCEEDED(hr)) {
        m_FxDevice = fxDevice;

        //
        // Drop the reference we got from CreateDevice.  Since this object
        // is partnered with the framework object they have the same 
        // lifespan - there is no need for an additional reference.
        //

        fxDevice->Release();
    }

    return hr;
}

HRESULT CSCEUsbDevice::Configure(VOID) {
    //Create IoQueueMgr
    HRESULT hr = CIoQueueMgr::CreateInstance(this, &m_IoQueue);
    if (FAILED(hr)) {
        Trace(TRACE_LEVEL_ERROR, "%!FUNC! : %!HRESULT! when creating IoQueueMgr instance.", hr);
        return hr;
    }

    hr = m_IoQueue->Configure();
    if (FAILED(hr)) {
        Trace(TRACE_LEVEL_ERROR, "%!FUNC! : %!HRESULT! when configuring IoQueueMgr.", hr);
        return hr;
    }

    hr = m_FxDevice->CreateDeviceInterface(&GUID_DEVINTERFACE_PSVDEVKIT, NULL);
    if (FAILED(hr)) {
        Trace(TRACE_LEVEL_ERROR, "%!FUNC! : %!HRESULT! when creating device interface.", hr);
        return hr;
    }

    Trace(TRACE_LEVEL_INFORMATION, "Created device interface with GUID %!GUID! successfully.", &GUID_DEVINTERFACE_PSVDEVKIT);
    return S_OK;
}

HRESULT CSCEUsbDevice::QueryInterface(_In_ REFIID InterfaceId, _Out_ PVOID* Object) {
    *Object = NULL;
    if (IsEqualIID(InterfaceId, __uuidof(IPnpCallback)))
    {
        *Object = QueryIPnpCallback();
        return S_OK;
    }
    else if (IsEqualIID(InterfaceId, __uuidof(IPnpCallbackHardware))) {
        *Object = QueryIPnpCallbackHardware();
        return S_OK;
    }
    else if (IsEqualIID(InterfaceId, __uuidof(IFileCallbackCleanup))) {
        *Object = QueryIFileCallbackCleanup();
        return S_OK;
    }
    else return CUnknown::QueryInterface(InterfaceId, Object);
}

HRESULT CSCEUsbDevice::SetConfigurationSync(USHORT configValue) {
    IWDFIoRequest* ioReq = NULL;
    IWDFRequestCompletionParams* completionParams = NULL;
    HRESULT hr = m_FxDevice->CreateRequest(NULL, NULL, &ioReq);
    if (SUCCEEDED(hr)) {
        WINUSB_SETUP_PACKET setupPacket;
        setupPacket.RequestType = BMREQUEST_STANDARD; //Not sure if this is the good macro
        setupPacket.Request = USB_REQUEST_SET_CONFIGURATION;
        setupPacket.Value = configValue;
        setupPacket.Index = 0;
        setupPacket.Length = 0;
        hr = m_FxUsbDevice->FormatRequestForControlTransfer(ioReq, &setupPacket, NULL, NULL);
        if (SUCCEEDED(hr)) {
            hr = ioReq->Send(m_FxUsbDevice, WDF_REQUEST_SEND_OPTION_SYNCHRONOUS, 0);
            if (SUCCEEDED(hr)) {
                
                ioReq->GetCompletionParams(&completionParams);
                hr = completionParams->GetCompletionStatus();
                Trace(TRACE_LEVEL_INFORMATION, "%!FUNC! : Request result is %!HRESULT!.", hr);
            }
            else {
                Trace(TRACE_LEVEL_ERROR, "%!FUNC! : %!HRESULT! when sending request.", hr);
            }
        }
        else {
            Trace(TRACE_LEVEL_ERROR, "%!FUNC! : %!HRESULT! when formating request.", hr);
        }
    }
    else {
        Trace(TRACE_LEVEL_ERROR, "%!FUNC! : %!HRESULT! when creating request.", hr);
    }
    SAFE_RELEASE(completionParams);
    SAFE_RELEASE(ioReq);
    return hr;
}

void CSCEUsbDevice::StopInput(VOID) {
    if (m_isInNonD0State) {
        Trace(TRACE_LEVEL_INFORMATION, "StopInput(do nothing)");
        m_Tracer->Log("StopInput(do nothing)");
    }
    else {
        m_Tracer->Log("StopInput");
        m_IoQueue->unk58 = FALSE;
        HRESULT hr = m_pIoTargetInterruptPipeStateMgmt->Stop(WdfIoTargetCancelSentIo);
        if (FAILED(hr)) {
            Trace(TRACE_LEVEL_ERROR, "%!FUNC! : %!HRESULT! when stoping interrupt pipe.", hr);
        }
        m_isInNonD0State = true;
    }
}

HRESULT CSCEUsbDevice::CreateUsbIoTargets(VOID) {
    IWDFUsbTargetFactory* pIUsbTargetFactory = NULL;
    IWDFUsbTargetDevice* pIUsbTargetDevice = NULL;
    IWDFUsbInterface* pIUsbInterface = NULL;

    UCHAR NumEndPoints = 0;

    HRESULT hr = m_FxDevice->QueryInterface(IID_PPV_ARGS(&pIUsbTargetFactory));
    if (FAILED(hr)) {
        Trace(TRACE_LEVEL_ERROR, "%!FUNC! : %!HRESULT! when getting USB target factory.", hr);
    }

    if (SUCCEEDED(hr)) {
        hr = pIUsbTargetFactory->CreateUsbTargetDevice(&pIUsbTargetDevice);
        if (FAILED(hr)) {
            Trace(TRACE_LEVEL_ERROR, "%!FUNC! : %!HRESULT! when creating USB target device.", hr);
        }
        else {
            m_FxUsbDevice = pIUsbTargetDevice;
            
            // Release the creation reference as object tree will maintain a reference
            pIUsbTargetDevice->Release();
        }
    }

    if (SUCCEEDED(hr)) {
        Trace(TRACE_LEVEL_INFORMATION, "%!FUNC! : Device has %d interfaces (expected 1).", pIUsbTargetDevice->GetNumInterfaces());
        
        hr = pIUsbTargetDevice->RetrieveUsbInterface(0, &pIUsbInterface);
        if (FAILED(hr)) {
            Trace(TRACE_LEVEL_ERROR, "%!FUNC! : %!HRESULT! when retrieving USB interface.", hr);
        }
        else {
            m_pIUsbInterface = pIUsbInterface;

            // Release the creation reference as object tree will maintain a reference
            pIUsbInterface->Release();
        }
    }

    if (SUCCEEDED(hr)) {
        IWDFUsbTargetPipe* pIUsbPipe = NULL;

        NumEndPoints = pIUsbInterface->GetNumEndPoints();
        Trace(TRACE_LEVEL_INFORMATION, "%!FUNC! : Interface 0 has %d end points.", NumEndPoints);
        for (UCHAR PipeIndex = 0; PipeIndex < NumEndPoints; PipeIndex++) {
            hr = pIUsbInterface->RetrieveUsbPipeObject(PipeIndex, &pIUsbPipe);

            if (FAILED(hr)) {
                Trace(TRACE_LEVEL_ERROR, "%!FUNC! : %!HRESULT! when retrieving USB pipe at index %d.", hr, PipeIndex);
                continue;
            }

            if (pIUsbPipe->IsOutEndPoint() && pIUsbPipe->GetType() == UsbdPipeTypeBulk) {
                m_pIUsbOutputPipe = pIUsbPipe;
            }
            //This part looks and feels awful, there should be a special case for UsbdPipeTypeInterrupt... and yet, the ASM does that...
            else if (pIUsbPipe->IsInEndPoint() && pIUsbPipe->GetType() == UsbdPipeTypeBulk) { 
                m_pIUsbInputPipe = pIUsbPipe;
                WUDF_DRIVER_ASSERT(m_pIoTargetInterruptPipeStateMgmt == NULL);

                hr = pIUsbPipe->QueryInterface(IID_PPV_ARGS(&m_pIoTargetInterruptPipeStateMgmt));
                if (FAILED(hr)) {
                    Trace(TRACE_LEVEL_ERROR, "%!FUNC! : %!HRESULT! when querying pipe at index %d for InterruptPipeTarget.", hr, PipeIndex);
                    m_pIoTargetInterruptPipeStateMgmt = NULL;
                }

                /* "good" (samples-following) implementation :
                if (pIUsbPipe->GetType() == UsbdPipeTypeInterrupt) {
                    m_pIUsbInterruptPipe = pIUsbPipe;
                    WUDF_DRIVER_ASSERT(m_pIoTargetInterruptPipe == NULL);
                    hr = pIUsbPipe->QueryInterface(IID_PPV_ARGS(&m_pIoTargetInterruptPipe));
                    if (FAILED(hr)) {
                        Trace(TRACE_LEVEL_ERROR, "%!FUNC! : %!HRESULT! when querying UsbdPipeTypeInterrupt pipe at index %d for InterruptPipeTarget.", hr, PipeIndex);
                        m_pIoTargetInterruptPipe = NULL;
                    }
                } 
                else if (pIUsbPipe->GetType() == UsbdPipeTypeBulk) {
                    m_pIUsbInputPipe = pIUsbPipe;
                }
                else pIUsbPipe->DeleteWdfObject();
                */
            }
            else {
                pIUsbPipe->DeleteWdfObject();
            }
        }
        Trace(TRACE_LEVEL_INFORMATION, "%!FUNC! : m_pIoTargetInterruptPipe is %s.", (m_pIoTargetInterruptPipeStateMgmt == NULL) ? "NULL" : "non-NULL");

        if (m_pIUsbInputPipe == NULL || m_pIUsbOutputPipe == NULL) {
            Trace(TRACE_LEVEL_ERROR, "%!FUNC! : InputPipe is %s, OutputPipe is %s.", 
                (m_pIUsbInputPipe == NULL) ? "NULL" : "non-NULL",
                (m_pIUsbOutputPipe == NULL) ? "NULL" : "non-NULL");

            SAFE_RELEASE(pIUsbTargetFactory);
            return E_UNEXPECTED;
        }

        USB_DEVICE_DESCRIPTOR usbDevDesc;
        ULONG bufLen = sizeof(usbDevDesc);
        hr = pIUsbTargetDevice->RetrieveDescriptor(USB_DEVICE_DESCRIPTOR_TYPE, 0, 0, &bufLen, &usbDevDesc);
        if (SUCCEEDED(hr)) {
            m_idProduct = usbDevDesc.idProduct;
            m_bcdDevice = usbDevDesc.bcdDevice;
        }
        else {
            Trace(TRACE_LEVEL_ERROR, "%!FUNC! : %!HRESULT! when retrieving device descriptor.", hr);
        }
    }
    SAFE_RELEASE(pIUsbTargetFactory);
    return hr;
}

HRESULT CSCEUsbDevice::ConfigureUsbPipes(VOID) {
    UCHAR policyValue = 1;
    HRESULT hr = m_pIUsbOutputPipe->SetPipePolicy(SHORT_PACKET_TERMINATE, 1, &policyValue);
    if (FAILED(hr)) {
        Trace(TRACE_LEVEL_ERROR, "%!FUNC! : %!HRESULT! when setting SHORT_PACKET_TERMINATE policy on output pipe.", hr);
    }
    return hr;
}

//IPnpCallback methods 
HRESULT STDMETHODCALLTYPE CSCEUsbDevice::OnD0Entry(_In_  IWDFDevice* pWdfDevice, _In_  WDF_POWER_DEVICE_STATE previousState) {
    UNREFERENCED_PARAMETER(previousState);
    UNREFERENCED_PARAMETER(pWdfDevice);

    Trace(TRACE_LEVEL_INFORMATION, "OnD0Entry()");
    m_Tracer->Log("D0Entry()");

    if (m_isInNonD0State) {
        if (IDPRODUCT_DEVKIT == m_idProduct && m_bcdDevice < BCDDEVICE_NEWREV) {
            HRESULT hr = SetConfigurationSync(0);
            if (FAILED(hr)) {
                Trace(TRACE_LEVEL_ERROR, "%!FUNC! : %!HRESULT! on SET_CONFIGURATION(0).", hr);
            }
            hr = SetConfigurationSync(1);
            if (FAILED(hr)) {
                Trace(TRACE_LEVEL_ERROR, "%!FUNC! : %!HRESULT! on SET_CONFIGURATION(1).", hr);
            }
        }

        Trace(TRACE_LEVEL_INFORMATION, "RestartInput");
        m_Tracer->Log("RestartInput");

        if (NULL == m_pIoTargetInterruptPipeStateMgmt) {
            Trace(TRACE_LEVEL_ERROR, "%!FUNC! : m_pIoTargetInterruptPipeStateMgmt is NULL.");
        }
        else {
            HRESULT hr = m_pIoTargetInterruptPipeStateMgmt->Start();
            if (FAILED(hr)) {
                Trace(TRACE_LEVEL_ERROR, "%!FUNC! : %!HRESULT! when starting interupt pipe.", hr);
            }
        }
        m_IoQueue->unk5C = 0x2;
        m_isInNonD0State = false;
    }
    else {
        Trace(TRACE_LEVEL_INFORMATION, "RestartInput(do nothing)");
        m_Tracer->Log("RestartInput(do nothing)");
    }
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CSCEUsbDevice::OnD0Exit(_In_  IWDFDevice* pWdfDevice, _In_  WDF_POWER_DEVICE_STATE newState) {
    UNREFERENCED_PARAMETER(newState);
    UNREFERENCED_PARAMETER(pWdfDevice);
    
    Trace(TRACE_LEVEL_INFORMATION, "OnD0Exit()");
    m_Tracer->Log("D0Exit()");
    StopInput();
    return S_OK;
}

VOID STDMETHODCALLTYPE CSCEUsbDevice::OnSurpriseRemoval(_In_  IWDFDevice* pWdfDevice) {
    UNREFERENCED_PARAMETER(pWdfDevice);
    Trace(TRACE_LEVEL_INFORMATION, "OnSurpriseRemoval()");
    m_Tracer->Log("OnSupRemv()");
}

HRESULT STDMETHODCALLTYPE CSCEUsbDevice::OnQueryRemove(_In_  IWDFDevice* pWdfDevice) {
    UNREFERENCED_PARAMETER(pWdfDevice);
    Trace(TRACE_LEVEL_INFORMATION, "OnQueryRemove()");
    m_Tracer->Log("OnQueRemv()");
    StopInput();
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CSCEUsbDevice::OnQueryStop(_In_  IWDFDevice* pWdfDevice) {
    UNREFERENCED_PARAMETER(pWdfDevice);
    Trace(TRACE_LEVEL_INFORMATION, "OnQueryStop()");
    m_Tracer->Log("OnQueStop()");
    return S_OK;
}

//IPnpCallbackHardware methods
HRESULT STDMETHODCALLTYPE CSCEUsbDevice::OnPrepareHardware(_In_  IWDFDevice* pWdfDevice) {
    UNREFERENCED_PARAMETER(pWdfDevice);
    Trace(TRACE_LEVEL_INFORMATION, "OnPrepareHardware()");
    m_Tracer->Log("PrepareH/W()");


    DWORD dwDeviceNameLength = 0;
    PWSTR pDeviceName = NULL;
    HRESULT hr = m_FxDevice->RetrieveDeviceName(NULL, &dwDeviceNameLength);
    if (FAILED(hr)) {
        Trace(TRACE_LEVEL_ERROR, "%!FUNC! : %!HRESULT! when retrieving device name length.", hr);
        goto free_and_return;
    }
    
    pDeviceName = new WCHAR[dwDeviceNameLength];
    if (pDeviceName == NULL) {
        Trace(TRACE_LEVEL_ERROR, "%!FUNC! : Could not allocate space for device name.");
        goto free_and_return;
    }

    hr = m_FxDevice->RetrieveDeviceName(pDeviceName, &dwDeviceNameLength);
    if (FAILED(hr)) {
        Trace(TRACE_LEVEL_ERROR, "%!FUNC! : %!HRESULT! when retrieving device name.", hr);
        goto free_and_return;
    }

    Trace(TRACE_LEVEL_INFORMATION, "%!FUNC! : Device name is %S.", pDeviceName);
    
    hr = CreateUsbIoTargets();
    if (FAILED(hr)) {
        Trace(TRACE_LEVEL_ERROR, "%!FUNC! : %!HRESULT! when creating USB I/O targets.", hr);
        goto free_and_return;
    }

    
    ULONG length = sizeof(m_Speed);
    hr = m_FxUsbDevice->RetrieveDeviceInformation(DEVICE_SPEED, &length, &m_Speed);
    if (FAILED(hr)) {
        Trace(TRACE_LEVEL_ERROR, "%!FUNC! : %!HRESULT! when retrieving device speed.", hr);
        goto free_and_return;
    }
    
    Trace(TRACE_LEVEL_INFORMATION, "%!FUNC! : Device runs at speed %u (%s).", m_Speed,
        (m_Speed == LowSpeed) ? "Low Speed" : (m_Speed == FullSpeed) ? "Full Speed" : (m_Speed == HighSpeed) ? "High Speed" : "???");

    m_Tracer->Log((m_Speed == HighSpeed) ? "Speed:HS" : "Speed:**FS**");

    hr = ConfigureUsbPipes();
    if (FAILED(hr)) {
        Trace(TRACE_LEVEL_ERROR, "%!FUNC! : %!HRESULT! when configuring USB pipes.", hr);
    }

free_and_return:
    delete[] pDeviceName;
    return hr;
}

HRESULT STDMETHODCALLTYPE CSCEUsbDevice::OnReleaseHardware(_In_  IWDFDevice* pWdfDevice) {
    UNREFERENCED_PARAMETER(pWdfDevice);
    Trace(TRACE_LEVEL_INFORMATION, "OnReleaseHardware()");
    m_Tracer->Log("ReleaseH/W()");

    SAFE_RELEASE(m_FxUsbDevice);
    return S_OK;
}

//IFileCallbackCleanup methods
VOID STDMETHODCALLTYPE CSCEUsbDevice::OnCleanupFile(_In_  IWDFFile* pWdfFileObject) {
    m_IoQueue->GetDevice()->m_pIUsbInputPipe->CancelSentRequestsForFile(pWdfFileObject);
    m_IoQueue->GetDevice()->m_pIUsbOutputPipe->CancelSentRequestsForFile(pWdfFileObject);
    m_IoQueue->GetDevice()->m_pIUsbInputPipe->Release();
    m_IoQueue->GetDevice()->m_pIUsbOutputPipe->Release();
}
