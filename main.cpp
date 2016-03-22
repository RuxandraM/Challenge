#include <stdio.h>
#include "Source/Utils.h"
#include "Source/SharedBuffer.h"
#include "Source/RM_Process.h"
#include "Source/RM_SharedMemory.h"
#include "Source/RM_MessageManager.h"

static u_int g_iPID = 0;



int main()
{
	g_iPID = _getpid();
	
	//------------------------------------------------------------------------
	//create the shared memory for all the processes
	RM_SharedMemory xSharedMemory;
	RM_SharedMemory xSharedLabelsMemory;
	xSharedMemory.Create(RM_ACCESS_READ | RM_ACCESS_WRITE, SHARED_MEMORY_MAX_SIZE, TEXT(SHARED_MEMORY_NAME), g_iPID);
	xSharedLabelsMemory.Create(RM_ACCESS_READ | RM_ACCESS_WRITE, SHARED_MEMORY_LABESLS_MAX_SIZE, TEXT(SHARED_MEMORY_LABESLS_NAME), g_iPID);
	RM_MessageManager<RM_CHALLENGE_PROCESS_COUNT, RM_WToRMessageData> xMessageManager;
	xMessageManager.Create(g_iPID);
	
	RM_Job xJob;
	xJob.Initialise(RM_CHALLENGE_PROCESS_COUNT);

	RM_CustomProcess xProcesses[RM_CHALLENGE_PROCESS_COUNT];
	xProcesses[RM_CHALLENGE_PROCESS_WRITER].Initialise(L"Writer");
	xProcesses[RM_CHALLENGE_PROCESS_FIRSTREADER].Initialise(L"FirstReader");
	xProcesses[RM_CHALLENGE_PROCESS_SECONDREADER].Initialise(L"SecondReader");
	for (u_int u = 0; u < RM_CHALLENGE_PROCESS_COUNT; ++u)
	{
		xJob.AddProcess(&xProcesses[u]);
	}

	printf("processes launched; waiting for key press (2) \n");
	WaitKeyPress(2);

	xJob.Shutdown();

	return 0;
}