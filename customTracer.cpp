#include "customTracer.h"

CCustomTracer::CCustomTracer() {
	//Original driver is weird here, it seemingly zero'es out the critsec AFTER initializing it...
	static_assert(sizeof(CustomTracerExportedData) == 0x20020, "Size of exported data struct is invalid.");
	static_assert(sizeof(expData.buf) == 0x20000, "Size of exported tracing buffer is invalid.");

	currentIdx = 0;

	expData.bufMaxSize = sizeof(expData.buf);
	expData.bufCurIdx = 0;

	memset(&cs, 0, sizeof(cs));
	memset(&expData.perfFreq, 0, sizeof(expData.perfFreq));
	memset(expData.unk48, 0, sizeof(expData.unk48));
	memset(expData.buf, 0, sizeof(expData.buf));

	InitializeCriticalSection(&cs);
	QueryPerformanceFrequency(&expData.perfFreq);
}

VOID CCustomTracer::AddDataToBuffer(_In_ CHAR str[], _In_ SIZE_T size) {
	if (size > 0) {
		do {
			SIZE_T availableSize = LAST_IDX(expData.buf) - currentIdx;
			SIZE_T copySize = (availableSize < size) ? availableSize : size;
			memcpy(&expData.buf[currentIdx], str, copySize);
			str += copySize;
			size -= copySize;
			currentIdx += copySize;

			if (currentIdx >= LAST_IDX(expData.buf))
				currentIdx = 0;

			expData.buf[currentIdx] = '\0';
		} while (size > 0);
	}
	expData.bufCurIdx = (UINT32)currentIdx;
}