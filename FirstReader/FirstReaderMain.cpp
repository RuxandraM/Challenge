#include <stdio.h>
#include <windows.h>
#include "..\Source\Utils.h"
#include "..\Source\RM_SharedBuffer.h"
#include "..\Source\RM_SharedMemory.h"
#include "..\Source\RM_MessageManager.h"

static int g_iPID = 0;
static u_int g_uLastTag = 0u;
static u_int g_uLastSegmentRead = 0u;
static int g_iProcessIndex = 1; //TODO: this is hardcoded; it should be sent at creation
static int g_iSenderIndex = 0;	//listen for messages from process 0




static void ReadData(RM_SharedBuffer& xSharedBuffer, RM_MessageManager<RM_CHALLENGE_PROCESS_COUNT, RM_WToRMessageData>& xMessageManager)
{
	while (1)
	{
		RM_WToRMessageData xMessage;
		RM_RETURN_CODE xResult = xMessageManager.BlockingReceiveWithFlush(g_iProcessIndex, &xMessage);
		if (xResult != RM_SUCCESS)
		{
			printf("[%d] Failed to read signalled message.",g_iPID);
			continue;	//go back and wait for other messages. It was a problem in the transmition medium
		}

		printf("[%d] Received signal for segment %d \n",g_iPID, xMessage.m_uSegmentWritten);

		if (xMessage.m_uTag - g_uLastTag >= NUM_SEGMENTS)
		{
			//we lost data. The writer was too quick and I wasn't listening for new events because I was busy writing out data
			//attempt to read all the data from the buffer
			g_uLastSegmentRead = (xMessage.m_uSegmentWritten + 1) % NUM_SEGMENTS;
		}
		
		//int iWriteTag = xSharedBuffer.GetTimestamp();
		//printf("Received signal with timestamp %d \n",iWriteTag);
		

		//I was signalled that there is some data to read
		u_int uCurrentSegment = g_uLastSegmentRead;
		u_int uMaxNumIterations = xMessage.m_uSegmentWritten >= g_uLastSegmentRead ?
			xMessage.m_uSegmentWritten - g_uLastSegmentRead :
			NUM_SEGMENTS - g_uLastSegmentRead + xMessage.m_uSegmentWritten;
		
		u_int uGUGU = uMaxNumIterations;
		//read all the segments from the last one I read to the one I was messaged about,
		//or the last N if the writer is too far ahead
		for (u_int u = 0; u < uMaxNumIterations;++u)
		{
			--uMaxNumIterations;
			u_int uSegmentTag = (u_int)xSharedBuffer.GetSegmentTimestamp(uCurrentSegment);
			//GUGU - the writer will reset the u_int at some point
			//if (uSegmentTag < g_uLastTag)
			//{
			//	break;
			//}
			
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
		g_uLastTag = xMessage.m_uTag;
		g_uLastSegmentRead = uCurrentSegment;
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

	RM_SharedMemory xSharedMemory;
	void* pSharedMemory = xSharedMemory.OpenMemory(RM_ACCESS_READ, SHARED_MEMORY_MAX_SIZE, TEXT(SHARED_MEMORY_NAME), g_iPID);
	RM_SharedMemory xSharedMemoryLabels;
	void* pSharedMemoryLabels = xSharedMemoryLabels.OpenMemory(RM_ACCESS_WRITE | RM_ACCESS_READ, SHARED_MEMORY_LABESLS_MAX_SIZE,
		TEXT(SHARED_MEMORY_LABESLS_NAME), g_iPID);
	
	RM_MessageManager<RM_CHALLENGE_PROCESS_COUNT, RM_WToRMessageData> xMessageManager;
	xMessageManager.Initialise(g_iPID, MESSAGE_CHANNELS1_SHARED_MEMORY_NAME);

	RM_SharedBuffer xSharedBuffer;
	xSharedBuffer.MapMemory(pSharedMemory, pSharedMemoryLabels);

	//GUGU
	ReadData(xSharedBuffer, xMessageManager);

	printf("First Reader done. Waiting key press to start... \n");
	WaitKeyPress(3);

	return S_OK;
}