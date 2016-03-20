#include <stdio.h>
#include <windows.h>
#include "..\Source\Utils.h"
#include "..\Source\SharedBuffer.h"

static int g_iPID = 0;
static int g_iLastTag = 0;
static int g_iLastSegmentRead = 0;

static void ReceivedSignal130(int signal)
{
	if (signal == SIGINT)
	{
		printf("[%d] Received signal from writer!! \n", g_iPID);
	}
	else 
	{
		
	}
}

static u_int g_uIsReading = 0;

static void ReadData(SharedBuffer& xSharedBuffer, HANDLE xEvent)
{
	while (1)
	{
		WaitForSingleObject(xEvent, INFINITE);
		//if (g_uIsReading) continue;
		g_uIsReading = 1;
		int iWriteTag = xSharedBuffer.GetTimestamp();
		//printf("Received signal with timestamp %d \n",iWriteTag);
		int iSegmentWritten = xSharedBuffer.GetLastSegmentWrittenIndex();
		printf("Received signal for segment %d \n", iSegmentWritten);

		//I was signalled that there is some data to read
		int uCurrentSegment = g_iLastSegmentRead;
		int uMaxNumIterations = iSegmentWritten >= g_iLastSegmentRead ?
			iSegmentWritten - g_iLastSegmentRead :
			NUM_SEGMENTS - g_iLastSegmentRead + iSegmentWritten;
		u_int uGUGU = uMaxNumIterations;
		while (uMaxNumIterations >= 0)
		{
			--uMaxNumIterations;
			int iSegmentTag = xSharedBuffer.GetSegmentTimestamp(uCurrentSegment);
			if (iSegmentTag < g_iLastTag)
			{
				break;
			}
			//printf("Trying to read segment %d \n", uCurrentSegment);
			//---------------------------------------------------------
			const char* pSegmentMemory = xSharedBuffer.GetSegmentForReading(uCurrentSegment);

			if (!pSegmentMemory)
			{
				//GUGU: TODO
				//continue;
				//Data Lost because the writer was too fast
				printf("First Reader: Skip %d \n", uCurrentSegment);
			}
			else
			{
				//I grabbed the pointer to the segment I want to read! Spawn a thread that copies this to an HDF5

				char pGUGUTest[10];
				memcpy(pGUGUTest, pSegmentMemory, 9);
				xSharedBuffer.SetFinishedReading(uCurrentSegment);
				pGUGUTest[9] = '\0';
				//delay the reader too much!!
				
				printf("First Reader: %s \n", pGUGUTest);
				//Sleep(2200);
			}
			//---------------------------------------------------------
			uCurrentSegment = (uCurrentSegment + 1) % NUM_SEGMENTS;
		}
		printf("------------------- Read %d iterations, last read %d \n", uGUGU, uCurrentSegment);
		g_iLastTag = iWriteTag;
		g_iLastSegmentRead = uCurrentSegment;
		g_uIsReading = 0;
		//g_uLastTimestamp = iWriteTag;
	}
#if GUGU_OLD
	unsigned char cCurrentIterationVal = 0;
	for (u_int uCurrentSegment = 0; uCurrentSegment < 10u; ++uCurrentSegment)
	{
		uCurrentSegment = uCurrentSegment % NUM_SEGMENTS;

		const char* pSegmentMemory = xSharedBuffer.GetSegmentForReading(uCurrentSegment);

		if (!pSegmentMemory)
		{
			//GUGU: TODO
			continue;
		}

		char pGUGUTest[10];
		memcpy(pGUGUTest, pSegmentMemory, 9);
		xSharedBuffer.SetFinishedReading(uCurrentSegment);
		pGUGUTest[9] = '\0';

		printf("First Reader: %s \n", pGUGUTest);
	}

#endif//GUGU_OLD
}

int main()
{
	g_iPID = _getpid();
	printf("[%d] First Reader initialised. Waiting key press to start... \n", g_iPID);
	//signal(SIGINT, ReceivedSignal130);
	HANDLE xEvent = OpenEvent(EVENT_ALL_ACCESS, FALSE, CHALLENGE_EVENT);
	//WaitForSingleObject(xEvent, INFINITE);
	//RegisterWaitForSingleObject(&xEvent, hTimer, (WAITORTIMERCALLBACK)CDR, NULL, INFINITE, WT_EXECUTEDEFAULT);
	//WaitKeyPress(3);

	long long llSharedMemMaxSize = SHARED_MEMORY_MAX_SIZE;

	TCHAR szName[] = TEXT(SHARED_MEMORY_NAME);
	//GUGU!!! - FILE_MAP_ALL_ACCESS - I need 2 shared memories :((
	HANDLE hMapFile = OpenFileMapping(
		FILE_MAP_ALL_ACCESS,			// read access
		FALSE,                 // do not inherit the name
		szName);               // name of mapping object

	if (hMapFile == nullptr)
	{
		printf("First Reader: Failed to open file mapping object (%d).\n", GetLastError());
		WaitKeyPress(3);
		return S_FALSE;
	}

	LPCTSTR pSharedMemory = (LPTSTR)MapViewOfFile(hMapFile,   // handle to map object
		FILE_MAP_ALL_ACCESS, // read permission
		0,
		0,
		llSharedMemMaxSize);

	if (pSharedMemory == nullptr)
	{
		printf("pid [%d] Failed to map shared memory! Quitting. \n", g_iPID);
		CloseHandle(hMapFile);
		WaitKeyPress(3);
		return S_FALSE;
	}

	SharedBuffer xSharedBuffer(const_cast<WCHAR*>(pSharedMemory));
	


	ReadData(xSharedBuffer,xEvent);

	printf("First Reader done. Waiting key press to start... \n");
	WaitKeyPress(3);
	//ConsoleKeyInfo cki = Console::ReadKey();
	

	UnmapViewOfFile(pSharedMemory);
	CloseHandle(hMapFile);

	

	return S_OK;
}