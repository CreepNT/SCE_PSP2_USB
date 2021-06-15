#pragma once

#define WINDOWS_LEAN_AND_MEAN 1
#include <Windows.h>
#include <Unknwnbase.h>

#include "comsup.h"
#include <cstdio> //sprintf_s

#define EXPOSED_DATA_HEADER_SIZE 0x20
#define EXPOSED_TRACE_BUFFER_SIZE 0x20000
#define EXPOSED_DATA_SIZE (EXPOSED_DATA_HEADER_SIZE + EXPOSED_TRACE_BUFFER_SIZE)

#define LAST_IDX(x) (sizeof(x) - 1)

//#pragma pack(push, 1)
typedef struct CustomTracerExportedData {
	ULONG bufMaxSize;
	ULONG bufCurIdx;
	LARGE_INTEGER perfFreq;
	UCHAR unk48[0x10];
	CHAR buf[EXPOSED_TRACE_BUFFER_SIZE];
} CustomTracerExportedData;
//#pragma pack(pop)

class CCustomTracer : public CUnknown {
//Public data
public:
	CustomTracerExportedData expData;

//Private data
private:
	CRITICAL_SECTION cs;
	SIZE_T currentIdx;
	//UINT32 unk;

//Private methods
private:
	VOID AddDataToBuffer(_In_ CHAR str[], _In_ SIZE_T size);

//Public methods
public:
	CCustomTracer();

	virtual ~CCustomTracer() {
		DeleteCriticalSection(&cs);
	}

	//Adds a log entry to the object's buffer
	VOID STDMETHODCALLTYPE Log(_In_ CHAR str[]) {
		//Prepare a string that contains the value of the performance counter
		LARGE_INTEGER perfCtr;
		CHAR perfCtrString[0x12];
		QueryPerformanceCounter(&perfCtr);
		sprintf_s<sizeof(perfCtrString)>(perfCtrString, "%016I64X:", (UINT64)perfCtr.QuadPart);
		
		size_t stringLen = strlen(str); //This is originally done in the critical section... why ? No one will sadly ever know...

		EnterCriticalSection(&cs);
		AddDataToBuffer(perfCtrString, sizeof(perfCtrString) - 1 /*don't add NUL*/);
		AddDataToBuffer(str, stringLen);
		LeaveCriticalSection(&cs);
	}

	virtual ULONG STDMETHODCALLTYPE AddRef(VOID) {
		return __super::AddRef();
	}

	_At_(this, __drv_freesMem(object))
	virtual ULONG STDMETHODCALLTYPE Release(VOID) {
		return __super::Release();
	}

	virtual HRESULT STDMETHODCALLTYPE QueryInterface(_In_ REFIID InterfaceId, _Out_ PVOID* Object) {
		return __super::QueryInterface(InterfaceId, Object);
	}
};

typedef class CCustomTracer* PCCustomTracer;