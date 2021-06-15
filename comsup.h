#pragma once

//
// Forward type declarations.  They are here rather than in internal.h as
// you only need them if you choose to use these support classes.
//

typedef class CUnknown *PCUnknown;
typedef class CClassFactory *PCClassFactory;

//
// Base class to implement IUnknown.  You can choose to derive your COM
// classes from this class, or simply implement IUnknown in each of your
// classes.
//

//Class implementation of IUnknown
class CUnknown : public IUnknown 
{

//
// Private data members and methods.  These are only accessible by the methods 
// of this class.
//
private:

    //
    // The reference count for this object.  Initialized to 1 in the
    // constructor.
    //

    LONG m_ReferenceCount;

//
// Protected data members and methods.  These are accessible by the subclasses 
// but not by other classes.
//
protected:
    //
    // The constructor and destructor are protected to ensure that only the 
    // subclasses of CUnknown can create and destroy instances.
    //
    CUnknown(void);

    //
    // The destructor MUST be virtual.  Since any instance of a CUnknown 
    // derived class should only be deleted from within CUnknown::Release, 
    // the destructor MUST be virtual or only CUnknown::~CUnknown will get
    // invoked on deletion.
    //
    // If you see that your CMyDevice specific destructor is never being
    // called, make sure you haven't deleted the virtual destructor here.
    //
    virtual ~CUnknown(void) {}

//
// Public Methods.  These are accessible by any class.
//
public:

    IUnknown* QueryIUnknown(void);

//
// COM Methods.
//
public:

    //
    // IUnknown methods
    //

    virtual ULONG STDMETHODCALLTYPE AddRef(VOID);
    
    virtual ULONG STDMETHODCALLTYPE Release(VOID);

    virtual HRESULT STDMETHODCALLTYPE QueryInterface(_In_ REFIID InterfaceId, _Out_ PVOID *Object);
};


//Class implementation of IClassFactory
class CClassFactory : public CUnknown, public IClassFactory 
{
private:
    static LONG s_LockCount;
    void* m_Tracer;

public:
    //Allow public creation
    CClassFactory(_In_ void* Tracer) : m_Tracer(Tracer) {
        static_cast<IUnknown*>(Tracer)->AddRef();
    }; 
    IClassFactory* QueryIClassFactory(VOID);

public: //COM Methods

    //
    // IUnknown methods
    //

    virtual ULONG STDMETHODCALLTYPE AddRef(VOID) {
        return __super::AddRef();
    }

    _At_(this, __drv_freesMem(object))
    virtual ULONG STDMETHODCALLTYPE Release(VOID) {
        return __super::Release();
    }

    virtual HRESULT STDMETHODCALLTYPE QueryInterface(_In_ REFIID InterfaceId, _Out_ PVOID *Object);

    //
    // IClassFactory methods.
    //

    virtual HRESULT STDMETHODCALLTYPE CreateInstance(
        _In_opt_ IUnknown *OuterObject,
        _In_ REFIID InterfaceId,
        _Out_ PVOID *Object);

    virtual HRESULT STDMETHODCALLTYPE LockServer(_In_ BOOL Lock);
};
