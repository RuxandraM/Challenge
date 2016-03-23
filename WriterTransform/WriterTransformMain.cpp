#include <stdio.h>
#include <windows.h>
#include "..\Source\Utils.h"
#include "..\Source\SharedBuffer.h"
#include "..\Source\RM_SharedMemory.h"
#include "..\Source\RM_StagingBuffer.h"
#include "..\Source\RM_MessageManager.h"
#include "..\Source\RM_Thread.h"
#include "..\Source\RM_Mutex.h"
#include "../Source/RM_ReaderOutputThread.h"
#include "../Source/RM_ThreadPool.h"
#include <vector>

static int g_iLastTag = 0;
static int g_iLastSegmentRead = 0;
static int g_iProcessIndex = 3; //TODO: this is hardcoded; it should be sent at creation

int main()
{

	//RM_SecondReader xReader;
	//xReader.SR_Initialise();

	//xReader.ReadData();

	//xReader.SR_Shutdown();

	printf("WriterTransform done. Waiting key press... \n");
	WaitKeyPress(3);

	return S_OK;
}