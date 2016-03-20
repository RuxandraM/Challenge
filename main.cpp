#include <stdio.h>
#include "Source/Utils.h"
#include "Source/SharedBuffer.h"
#include "Source/RM_Process.h"
#include "Source/RM_SharedMemory.h"

static u_int g_uPID = 0;

enum CHALLENGE_PROCESS
{
	CHALLENGE_PROCESS_WRITER,
	CHALLENGE_PROCESS_FIRSTREADER,
	CHALLENGE_PROCESS_COUNT
};



int main()
{
	g_uPID = _getpid();
	
	//------------------------------------------------------------------------
	//create the shared memory for all the processes
	RM_SharedMemory xSharedMemory;
	RM_SharedMemory xSharedLabelsMemory;
	xSharedMemory.Create(RM_ACCESS_READ | RM_ACCESS_WRITE, SHARED_MEMORY_MAX_SIZE, TEXT(SHARED_MEMORY_NAME), g_uPID);
	xSharedLabelsMemory.Create(RM_ACCESS_READ | RM_ACCESS_WRITE, SHARED_MEMORY_LABESLS_MAX_SIZE, TEXT(SHARED_MEMORY_LABESLS_NAME), g_uPID);


	HANDLE xResult = CreateEvent(nullptr, FALSE, FALSE, CHALLENGE_EVENT);
	if (xResult == nullptr)
	{
		printf("Main: Failed to create event (%d).\n", GetLastError());
		WaitKeyPress(2);
		return S_FALSE;
	}
	
	RM_Job xJob;
	xJob.Initialise(CHALLENGE_PROCESS_COUNT);

	RM_CustomProcess xProcesses[CHALLENGE_PROCESS_COUNT];
	xProcesses[CHALLENGE_PROCESS_WRITER].Initialise(L"Writer");
	xProcesses[CHALLENGE_PROCESS_FIRSTREADER].Initialise(L"FirstReader");
	for (u_int u = 0; u < CHALLENGE_PROCESS_COUNT; ++u)
	{
		xJob.AddProcess(&xProcesses[u]);
	}


	printf("processes launched; waiting for key press \n");
	WaitKeyPress(2);

	//xSharedMemory.CloseMemory();
	//xSharedLabelsMemory.CloseMemory();
	xJob.Shutdown();

	return 0;
}