#pragma once
#include "internal.h"

typedef class CIoQueueMgr* PCIoQueueMgr;

class CSCEUsbDevice : public CUnknown, public IPnpCallback, public IPnpCallbackHardware, public IFileCallbackCleanup {

// Private data members
private:
    IWDFDevice *m_FxDevice;
    PCIoQueueMgr m_IoQueue;
    IWDFUsbTargetDevice* m_FxUsbDevice;
    IWDFUsbInterface* m_pIUsbInterface;
    IWDFUsbTargetPipe* m_pIUsbInputPipe; //not always ?
    IWDFUsbTargetPipe* m_pIUsbOutputPipe;
    //IWDFUsbTargetPipe* m_pIUsbInterruptPipe;
    IWDFIoTargetStateManagement* m_pIoTargetInterruptPipeStateMgmt;
    UCHAR m_Speed;
    UCHAR m_SwitchState; //Not sure
    USHORT m_idProduct; //USB descriptor's PID
    USHORT m_bcdDevice; //USB descriptor's revision
    //USHORT unk;
    PCCustomTracer m_Tracer;
    bool m_isInNonD0State; //Checked in OnD0xxx callbacks 
    //UCHAR[7] padding?


// Private methods
private:
    CSCEUsbDevice(_In_ PCCustomTracer Tracer) : m_FxDevice(NULL), m_IoQueue(NULL), m_FxUsbDevice(NULL),
        m_pIUsbInterface(NULL), m_pIUsbInputPipe(NULL), m_pIUsbOutputPipe(NULL), m_pIoTargetInterruptPipeStateMgmt(NULL),
        m_Speed(0), m_SwitchState(0), m_idProduct(0), m_bcdDevice(0), m_Tracer(Tracer), m_isInNonD0State(false) {
        Tracer->AddRef();
    }

    virtual ~CSCEUsbDevice() {
        m_Tracer->Release();
    }

    HRESULT Initialize(_In_ IWDFDriver *FxDriver, _In_ IWDFDeviceInitialize *FxDeviceInit);

    IPnpCallback* QueryIPnpCallback(VOID) {
        __super::AddRef();
        return static_cast<IPnpCallback*>(this);
    }
    
    IPnpCallbackHardware* QueryIPnpCallbackHardware(VOID) {
        __super::AddRef();
        return static_cast<IPnpCallbackHardware*>(this);
    }

    IFileCallbackCleanup* QueryIFileCallbackCleanup(VOID) {
        __super::AddRef();
        return static_cast<IFileCallbackCleanup*>(this);
    }

    //Sends a SET_CONFIGURATION packet with provided configValue to USB synchronously
    HRESULT SetConfigurationSync(USHORT configValue);

    //Create the USB I/O targets
    HRESULT CreateUsbIoTargets(VOID);

    //Configure the USB I/O pipes
    HRESULT ConfigureUsbPipes(VOID);

    //Stop recieving input
    void StopInput(VOID);

// Public methods
public:
    // The factory method used to create an instance of this driver.
    static HRESULT CreateInstance(_In_ IWDFDriver *FxDriver, _In_ IWDFDeviceInitialize *FxDeviceInit, _In_ PCCustomTracer Tracer, _Out_ PCSCEUsbDevice *Device);

    IWDFDevice* GetFxDevice(VOID) {
        return m_FxDevice;
    }

    IWDFUsbTargetDevice* GetUsbTargetDevice(VOID) {
        return m_FxUsbDevice;
    }

    IWDFUsbTargetPipe* GetUsbInputPipe(VOID) {
        return m_pIUsbInputPipe;
    }

    IWDFUsbTargetPipe* GetUsbOutputPipe(VOID) {
        return m_pIUsbOutputPipe;
    }

    USHORT GetProductId(VOID) {
        return m_idProduct;
    }

    UCHAR GetSpeed(VOID) {
        return m_Speed;
    }

    //Returns a pointer to this device's Tracer, and adds a reference.
    PCCustomTracer QueryTracer(VOID) {
        m_Tracer->AddRef();
        return m_Tracer;
    }

    HRESULT Configure(VOID);

// COM methods
public:
    // IUnknown methods
    virtual ULONG STDMETHODCALLTYPE AddRef(VOID) {
        return __super::AddRef();
    }

    _At_(this, __drv_freesMem(object))
    virtual ULONG STDMETHODCALLTYPE Release(VOID) {
        return __super::Release();
    }

    virtual HRESULT STDMETHODCALLTYPE QueryInterface(_In_ REFIID InterfaceId, _Out_ PVOID* Object);

    //IPnpCallback methods 
    virtual HRESULT STDMETHODCALLTYPE OnD0Entry(_In_  IWDFDevice* pWdfDevice, _In_  WDF_POWER_DEVICE_STATE previousState);

    virtual HRESULT STDMETHODCALLTYPE OnD0Exit(_In_  IWDFDevice* pWdfDevice, _In_  WDF_POWER_DEVICE_STATE newState);

    virtual void STDMETHODCALLTYPE OnSurpriseRemoval(_In_  IWDFDevice* pWdfDevice);

    virtual HRESULT STDMETHODCALLTYPE OnQueryRemove(_In_  IWDFDevice* pWdfDevice);

    virtual HRESULT STDMETHODCALLTYPE OnQueryStop(_In_  IWDFDevice* pWdfDevice);

    //IPnpCallbackHardware methods
    virtual HRESULT STDMETHODCALLTYPE OnPrepareHardware(_In_  IWDFDevice* pWdfDevice);

    virtual HRESULT STDMETHODCALLTYPE OnReleaseHardware(_In_  IWDFDevice* pWdfDevice);

    //IFileCallbackCleanup methods
    virtual void STDMETHODCALLTYPE OnCleanupFile(_In_  IWDFFile* pWdfFileObject);
};
