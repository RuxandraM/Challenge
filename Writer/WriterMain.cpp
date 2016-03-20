#include <stdio.h>
#include <windows.h>
#include "..\Source\Utils.h"
#include "..\Source\SharedBuffer.h"

static int g_iPID = 0;
static int g_uTag = 0;

static void ReceivedSignal130(int signal)
{
	if (signal == SIGINT)
	{
		printf("[%d] Writer: received my own signal :( \n", g_iPID);
	}
	else
	{

	}
}

int main()
{
	g_iPID = _getpid();
	printf("Writing to console pid %d \n", g_iPID);
	HANDLE xEvent = OpenEvent(EVENT_ALL_ACCESS, FALSE, CHALLENGE_EVENT);
	

	long long llSharedMemMaxSize = SHARED_MEMORY_MAX_SIZE;

	TCHAR szName[] = TEXT(SHARED_MEMORY_NAME);
	//HANDLE hMapFile = CreateFileMapping(
	//	INVALID_HANDLE_VALUE,    // invalid means to share just memory, not a file
	//	NULL,                    // default security
	//	PAGE_READWRITE,          // read/write access
	//	0,                       // maximum object size (high-order DWORD)
	//	llSharedMemMaxSize,		 // maximum object size (low-order DWORD)
	//	szName);                 // name of mapping object

	HANDLE hMapFile = OpenFileMapping(
		FILE_MAP_ALL_ACCESS,   // read/write access
		FALSE,                 // do not inherit the name
		szName);               // name of mapping object

	if (hMapFile == nullptr)
	{
		printf("[%d] Writer: Failed to open file mapping object (%d).\n", g_iPID,GetLastError());
		WaitKeyPress(1);
		return S_FALSE;
	}

	LPCTSTR pSharedMemory = (LPTSTR)MapViewOfFile(hMapFile,   // handle to map object
		FILE_MAP_WRITE, // read/write permission
		0,
		0,
		llSharedMemMaxSize);

	if (pSharedMemory == nullptr)
	{
		printf("[%d] Writer: Failed to map shared memory! Quitting. \n", g_iPID);
		CloseHandle(hMapFile);
		WaitKeyPress(1);
		return S_FALSE;
	}

	printf("[%d] Writer initialised. Waiting key press to start... \n", g_iPID);
	WaitKeyPress(1);

	SharedBuffer xSharedBuffer(const_cast<WCHAR*>(pSharedMemory));
	xSharedBuffer.ClearFlags();

	//Segment xTemporarySegment;
	
	//GUGU: start by writing 10 segments
	unsigned char cCurrentIterationVal = 'a';
	for (u_int uCurrentSegment = 0; uCurrentSegment < 100u; ++uCurrentSegment)
	{
		uCurrentSegment = uCurrentSegment % NUM_SEGMENTS;
		printf("[%d] Writing to segment %d \n", g_iPID, uCurrentSegment);

		char* pSegmentMemory = xSharedBuffer.GetSegmentForWriting(uCurrentSegment);
		
		if (!pSegmentMemory)
		{
			//it means the shared buffer is in use for reading; I can't stop, I need to write data immediately
			printf("GUGU: TODO");
			WaitKeyPress(1);
			return S_FALSE;
		}

		char* pCurrentPtr = pSegmentMemory;
		//GUGU!!! memset!
		while ((pSegmentMemory + SEGMENT_SIZE) - pCurrentPtr > 0)
		{
			*pCurrentPtr = cCurrentIterationVal;
			++pCurrentPtr;
		}
		xSharedBuffer.SetFinishedWriting(uCurrentSegment,++g_uTag);
		SetEvent(xEvent);
		cCurrentIterationVal++;

		//GUGU - the trigger time must be 1 second after we start writing!
		Sleep(1000);
	}

	//signal(SIGINT, ReceivedSignal130);
	//raise(SIGINT);
	
	
	//SetEvent(xEvent);

	printf("Finished writing to the shared memory \n");
	WaitKeyPress(1);

	UnmapViewOfFile(pSharedMemory);

	CloseHandle(hMapFile);

	

	return S_OK;
}