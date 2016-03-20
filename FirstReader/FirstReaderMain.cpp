#include <stdio.h>
#include <windows.h>
#include "..\Source\Utils.h"
#include "..\Source\SharedBuffer.h"
#include "..\Source\RM_SharedMemory.h"
#include "..\Source\RM_MessageManager.h"

static int g_iPID = 0;
static int g_iLastTag = 0;
static int g_iLastSegmentRead = 0;
static int g_iProcessIndex = 1; //TODO: this is hardcoded; it should be sent at creation

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


static void ReadData(SharedBuffer& xSharedBuffer, RM_MessageManager<RM_CHALLENGE_PROCESS_COUNT>& xMessageManager)
{
	while (1)
	{
		int iWriteTag = xMessageManager.BlockingReceive(g_iProcessIndex);
		int iSegmentWritten = xSharedBuffer.GetLastSegmentWrittenIndex();
		printf("Received signal for segment %d \n", iSegmentWritten);

		if (iWriteTag - g_iLastTag >= NUM_SEGMENTS)
		{
			//we lost data. The writer was too quick and I wasn't listening for new events because I was busy writing out data
			//attempt to read all the data from the buffer
			g_iLastSegmentRead = (iSegmentWritten + 1) % NUM_SEGMENTS;
		}
		
		//int iWriteTag = xSharedBuffer.GetTimestamp();
		//printf("Received signal with timestamp %d \n",iWriteTag);
		

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
		//g_uIsReading = 0;
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
	//HANDLE xEvent = OpenEvent(EVENT_ALL_ACCESS, FALSE, CHALLENGE_EVENT);
	//WaitForSingleObject(xEvent, INFINITE);
	//RegisterWaitForSingleObject(&xEvent, hTimer, (WAITORTIMERCALLBACK)CDR, NULL, INFINITE, WT_EXECUTEDEFAULT);
	//WaitKeyPress(3);

	RM_SharedMemory xSharedMemory;
	const void* pSharedMemory = xSharedMemory.OpenMemory(RM_ACCESS_READ, SHARED_MEMORY_MAX_SIZE, TEXT(SHARED_MEMORY_NAME), g_iPID);
	RM_SharedMemory xSharedMemoryLabels;
	const void* pSharedMemoryLabels = xSharedMemoryLabels.OpenMemory(RM_ACCESS_WRITE | RM_ACCESS_READ, SHARED_MEMORY_LABESLS_MAX_SIZE,
		TEXT(SHARED_MEMORY_LABESLS_NAME), g_iPID);
	
	RM_MessageManager<RM_CHALLENGE_PROCESS_COUNT> xMessageManager;
	xMessageManager.Initialise(g_iPID);

	SharedBuffer xSharedBuffer(const_cast<void*>(pSharedMemory), const_cast<void*>(pSharedMemoryLabels));
	


	ReadData(xSharedBuffer, xMessageManager);

	printf("First Reader done. Waiting key press to start... \n");
	WaitKeyPress(3);
	
	//xSharedMemory.CloseMemory();
	//xSharedMemoryLabels.CloseMemory();

	return S_OK;
}