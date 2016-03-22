#include <stdio.h>
#include <windows.h>
#include "..\Source\Utils.h"
#include "..\Source\SharedBuffer.h"
#include "..\Source\RM_SharedMemory.h"
#include "..\Source\RM_MessageManager.h"

static int g_iPID = 0;
static u_int g_uTag = 0u;
static int g_iProcessIndex = 0;	//TODO: this is hardcoded; it should be sent at creation

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
	
	RM_SharedMemory xSharedMemory;
	void* pSharedMemory = xSharedMemory.OpenMemory(RM_ACCESS_WRITE, SHARED_MEMORY_MAX_SIZE, TEXT(SHARED_MEMORY_NAME), g_iPID);
	RM_SharedMemory xSharedMemoryLabels;
	void* pSharedMemoryLabels = xSharedMemoryLabels.OpenMemory(RM_ACCESS_WRITE|RM_ACCESS_READ, SHARED_MEMORY_LABESLS_MAX_SIZE,
		TEXT(SHARED_MEMORY_LABESLS_NAME), g_iPID);
	RM_MessageManager<RM_CHALLENGE_PROCESS_COUNT, RM_WToRMessageData> xMessageManager;
	xMessageManager.Initialise(g_iPID);


	printf("[%d] Writer initialised. Waiting key press (1) to start... \n", g_iPID);
	WaitKeyPress(1);

	SharedBuffer xSharedBuffer(pSharedMemory, pSharedMemoryLabels);
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
		xSharedBuffer.SetFinishedWriting(uCurrentSegment,g_uTag);
		RM_WToRMessageData xMessage = { g_uTag, uCurrentSegment };
		RM_RETURN_CODE xResult = xMessageManager.SendMessage(-1, g_iProcessIndex, &xMessage);
		if (xResult != RM_SUCCESS)
		{
			//wait a little and try again for a few times (say 10 times, this param can be exposed)
			for (u_int u = 0; u < 10; ++u)
			{
				Sleep(5);
				xResult = xMessageManager.SendMessage(-1, g_iProcessIndex, &xMessage);
				if (xResult == RM_SUCCESS) break;
			}
			if (xResult != RM_SUCCESS)
			{
				printf("Failed to send message. The written data will never be read!");
				//TODO: could try looking for new readers, or we could increase buffer sizes - 
				//maybe the readers are too slow and fill up all the space?!
			}
		}

		cCurrentIterationVal++;

		//GUGU - the trigger time must be 1 second after we start writing!
		Sleep(1000);
	}

	//signal(SIGINT, ReceivedSignal130);
	//raise(SIGINT);
	
	

	printf("Finished writing to the shared memory \n");
	WaitKeyPress(1);

	return S_OK;
}