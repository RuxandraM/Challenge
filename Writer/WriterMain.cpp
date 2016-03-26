#include <stdio.h>
#include <windows.h>
#include "../Source/Utils.h"
#include "../Source/RM_SharedBuffer.h"
#include "../Source/RM_SharedMemory.h"
#include "../Source/RM_MessageManager.h"
#include "../Source/RM_SharedInitData.h"

static int g_iPID = 0;
static int g_iProcessIndex = 0;	//TODO: this is hardcoded; it should be sent at creation


int main()
{
	g_iPID = _getpid();
	
	RM_SharedInitDataLayout xInitDataLayout;
	{
		RM_SharedMemory xSharedMemory;
		std::string xSharedInitDataName(SHARED_INIT_DATA_NAME);
		void* pSharedMemory = xSharedMemory.OpenMemory(RM_ACCESS_WRITE | RM_ACCESS_READ, SHARED_INIT_DATA_MAX_SIZE,
			xSharedInitDataName, g_iPID);
		xInitDataLayout.MapMemory(pSharedMemory);
	}

	const RM_InitData& xInitData = xInitDataLayout.GetInitData(g_iProcessIndex);

	RM_SharedBuffer xSharedBuffer;
	{
		RM_SharedMemory xSharedMemory;
		std::string xSharedBuffName(xInitData.pSharedBufferMemNameOut);
		void* pSharedMemory = xSharedMemory.OpenMemory(RM_ACCESS_WRITE, SHARED_MEMORY_MAX_SIZE, xSharedBuffName, g_iPID);
		
		std::string xLablesObjName(xSharedBuffName);
		xLablesObjName.append("Lables");
		RM_SharedMemory xSharedMemoryLabels;
		void* pSharedMemoryLabels = xSharedMemoryLabels.OpenMemory(RM_ACCESS_WRITE | RM_ACCESS_READ, SHARED_MEMORY_LABESLS_MAX_SIZE, xLablesObjName, g_iPID);
		xSharedBuffer.MapMemory(pSharedMemory, pSharedMemoryLabels);
	}

	RM_MessageManager<RM_CHALLENGE_PROCESS_COUNT, RM_WToRMessageData> xMessageManager;
	{	
		std::string xMsgQName(xInitData.pMessageQueueNameOut);
		xMessageManager.Initialise(g_iPID, xMsgQName);
	}
	

	//printf("[%d] Writer initialised. Waiting key press (1) to start... \n", g_iPID);
	//WaitKeyPress(1);
	
	const u_int uNumIterations = xInitData.m_uNumWriteIterations;
	
	u_int uCurrentSegment = 0;
	u_int uCurrentTag = 0;
	unsigned char cCurrentIterationVal = 'a';
	for (u_int uIteration = 0; uIteration < uNumIterations; ++uIteration)
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
		
		//TODO - the trigger time must be 1 second after we start writing!
		//Counting the time should be on a different thread if we want write signals at every second
		Sleep(1000);
	}

	printf("Finished writing to the shared memory \n");
	system("pause");

	return S_OK;
}