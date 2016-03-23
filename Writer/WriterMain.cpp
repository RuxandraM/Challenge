#include <stdio.h>
#include <windows.h>
#include "..\Source\Utils.h"
#include "..\Source\SharedBuffer.h"
#include "..\Source\RM_SharedMemory.h"
#include "..\Source\RM_MessageManager.h"

static int g_iPID = 0;
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
	
	RM_SharedMemory xSharedMemory;
	void* pSharedMemory = xSharedMemory.OpenMemory(RM_ACCESS_WRITE, SHARED_MEMORY_MAX_SIZE, TEXT(SHARED_MEMORY_NAME), g_iPID);
	RM_SharedMemory xSharedMemoryLabels;
	void* pSharedMemoryLabels = xSharedMemoryLabels.OpenMemory(RM_ACCESS_WRITE|RM_ACCESS_READ, SHARED_MEMORY_LABESLS_MAX_SIZE,
		TEXT(SHARED_MEMORY_LABESLS_NAME), g_iPID);
	RM_MessageManager<RM_CHALLENGE_PROCESS_COUNT, RM_WToRMessageData> xMessageManager;
	xMessageManager.Initialise(g_iPID);


	printf("[%d] Writer initialised. Waiting key press (1) to start... \n", g_iPID);
	WaitKeyPress(1);

	SharedBuffer xSharedBuffer;
	xSharedBuffer.MapMemory(pSharedMemory, pSharedMemoryLabels);
	
	
	//GUGU: start by writing 10 segments
	u_int uCurrentSegment = 0;
	u_int uCurrentTag = 0;
	unsigned char cCurrentIterationVal = 'a';
	for (u_int uGUGU = 0; uGUGU < 50u; ++uGUGU)
	{
		
		printf("[%d] Writing to segment %d [%c%c%c...] \n", g_iPID, uCurrentSegment, cCurrentIterationVal, cCurrentIterationVal, cCurrentIterationVal);

		char* pSegmentMemory = xSharedBuffer.GetSegmentForWriting(uCurrentSegment);
		
		if (!pSegmentMemory)
		{
			//it means the shared buffer is in use for reading; I can't stop, I need to write data immediately
			printf("The Shared buffer is not available for reading any longer. Consider increasing the sizes for the circular buffers/staging memory.\n");
			WaitKeyPress(1);
			return S_FALSE;
		}

		//write the data to the segment
		memset(pSegmentMemory, cCurrentIterationVal, SEGMENT_SIZE);
		
		//release access to that segment such that readers can start
		xSharedBuffer.SetFinishedWriting(uCurrentTag, uCurrentSegment);

		//tell all readers that data has been sent to them
		RM_WToRMessageData xMessage = { uCurrentTag, uCurrentSegment };
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

		++cCurrentIterationVal;	//this is our random character
		++uCurrentTag;
		uCurrentSegment = (uCurrentSegment + 1) % NUM_SEGMENTS;
		
		//GUGU
		//TODO - the trigger time must be 1 second after we start writing!
		//We could spawn a thread every second - the counting would be done on a different thread
		Sleep(1000);
	}
	

	printf("Finished writing to the shared memory \n");
	WaitKeyPress(1);

	return S_OK;
}