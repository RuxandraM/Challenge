#include <stdio.h>
#include <windows.h>
#include "..\Source\Utils.h"
#include "..\Source\SharedBuffer.h"
#include "..\Source\RM_SharedMemory.h"
#include "..\Source\RM_MessageManager.h"

#define READER_MAX_RANDOM_SEGMENTS 20

static int g_iPID = 0;
static int g_iLastTag = 0;
static int g_iLastSegmentRead = 0;
static int g_iProcessIndex = 2; //TODO: this is hardcoded; it should be sent at creation

static void ReadData(SharedBuffer& xSharedBuffer, RM_MessageManager<RM_CHALLENGE_PROCESS_COUNT>& xMessageManager)
{
	u_int uNumElememts = std::rand() % READER_MAX_RANDOM_SEGMENTS;


	int iWriteTag = xMessageManager.BlockingReceive(g_iProcessIndex);
	int iSegmentWritten = xSharedBuffer.GetLastSegmentWrittenIndex();
	printf("[%d] Received signal for segment %d, tag %d, attempting to read %d segment from now on \n", g_iPID, iSegmentWritten, iWriteTag, uNumElememts);
}

int main()
{
	g_iPID = _getpid();
	
	RM_SharedMemory xSharedMemory;
	const void* pSharedMemory = xSharedMemory.OpenMemory(RM_ACCESS_READ, SHARED_MEMORY_MAX_SIZE, TEXT(SHARED_MEMORY_NAME), g_iPID);
	RM_SharedMemory xSharedMemoryLabels;
	const void* pSharedMemoryLabels = xSharedMemoryLabels.OpenMemory(RM_ACCESS_WRITE | RM_ACCESS_READ, SHARED_MEMORY_LABESLS_MAX_SIZE,
		TEXT(SHARED_MEMORY_LABESLS_NAME), g_iPID);

	RM_MessageManager<RM_CHALLENGE_PROCESS_COUNT> xMessageManager;
	xMessageManager.Initialise(g_iPID);

	SharedBuffer xSharedBuffer(const_cast<void*>(pSharedMemory), const_cast<void*>(pSharedMemoryLabels));
	printf("[%d] Second Reader initialised. Waiting for the writer to start \n", g_iPID);



	ReadData(xSharedBuffer, xMessageManager);

	printf("First Reader done. Waiting key press to start... \n");
	WaitKeyPress(2);

	return S_OK;
}