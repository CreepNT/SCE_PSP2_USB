#include "internal.h"
#include "comsup.tmh"

LONG CClassFactory::s_LockCount = 0; //Static lock count


CUnknown::CUnknown(void) : m_ReferenceCount(1) {}

HRESULT STDMETHODCALLTYPE CUnknown::QueryInterface(_In_ REFIID InterfaceId, _Out_ PVOID *Object) {
    if (IsEqualIID(InterfaceId, __uuidof(IUnknown)))
    {
        *Object = QueryIUnknown();
        return S_OK;
    }
    else
    {
        *Object = NULL;
        return E_NOINTERFACE;
    }
}

IUnknown* CUnknown::QueryIUnknown(VOID) {
    AddRef();
    return static_cast<IUnknown *>(this);
}

ULONG STDMETHODCALLTYPE CUnknown::AddRef(VOID) {
    return InterlockedIncrement(&m_ReferenceCount);
}

ULONG STDMETHODCALLTYPE CUnknown::Release(VOID) {
    ULONG count = InterlockedDecrement(&m_ReferenceCount);

    if (count == 0)
    {
        delete this;
    }
    return count;
}

//
// Implementation of CClassFactory methods.
//

IClassFactory* CClassFactory::QueryIClassFactory(VOID){
    AddRef();
    return static_cast<IClassFactory *>(this);
}

HRESULT CClassFactory::QueryInterface(_In_ REFIID InterfaceId, _Out_ PVOID *Object) {
    //
    // This class only supports IClassFactory so check for that.
    //

    if (IsEqualIID(InterfaceId, __uuidof(IClassFactory))) {
        *Object = QueryIClassFactory();
        return S_OK;
    }
    else {
        //
        // See if the base class supports the interface.
        //
        return CUnknown::QueryInterface(InterfaceId, Object);
    }
}

HRESULT STDMETHODCALLTYPE CClassFactory::CreateInstance(_In_opt_ IUnknown* /* OuterObject */, _In_ REFIID InterfaceId, _Out_ PVOID *Object) {
    HRESULT hr;
    PCSCEUsbDriver driver;

    *Object = NULL;

    hr = CSCEUsbDriver::CreateInstance(static_cast<PCCustomTracer>(m_Tracer), &driver);

    if (SUCCEEDED(hr)) 
    {
        hr = driver->QueryInterface(InterfaceId, Object);
        driver->Release();
    }

    return hr;
}

HRESULT STDMETHODCALLTYPE CClassFactory::LockServer(_In_ BOOL Lock) {
    if (Lock)
        InterlockedIncrement(&s_LockCount);
    else 
        InterlockedDecrement(&s_LockCount);

    return S_OK;
}

#include <windows.h>