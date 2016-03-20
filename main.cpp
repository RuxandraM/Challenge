#include <stdio.h>
#include "Source/Utils.h"
#include "Source/SharedBuffer.h"




enum CHALLENGE_PROCESS
{
	CHALLENGE_PROCESS_WRITER,
	CHALLENGE_PROCESS_FIRSTREADER,
	CHALLENGE_PROCESS_COUNT
};



int main()
{
	//CustomProcess xProcessParent;
	//xProcessParent.Initialise(L"ProcessSpawner", nullptr);

	//------------------------------------------------------------------------------
	HANDLE xMainJobHandle = CreateJobObject(nullptr, L"NanoporeChallengeJobObject");

	//tell the job to force killing all the processes when it is done
	JOBOBJECT_EXTENDED_LIMIT_INFORMATION xExtendedInfo = {};
	xExtendedInfo.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
	JOBOBJECTINFOCLASS xJobObjectInfoClass = {};
	SetInformationJobObject(xMainJobHandle, xJobObjectInfoClass, (void*)&xExtendedInfo,
		(u_int)sizeof(JOBOBJECT_EXTENDED_LIMIT_INFORMATION));

	//------------------------------------------------------------------------
	
	

	//create shared memory
	long long llSharedMemMaxSize = SHARED_MEMORY_MAX_SIZE;

	TCHAR szName[] = TEXT(SHARED_MEMORY_NAME);
	HANDLE hMapFile = CreateFileMapping(
		INVALID_HANDLE_VALUE,    // invalid means to share just memory, not a file
		NULL,                    // default security
		PAGE_READWRITE,          // read/write access
		0,                       // maximum object size (high-order DWORD)
		llSharedMemMaxSize,		 // maximum object size (low-order DWORD)
		szName);                 // name of mapping object

	if (hMapFile == nullptr)
	{
		printf("Main: Failed to create file mapping object (%d).\n", GetLastError());
		WaitKeyPress(2);
		return S_FALSE;
	}
	HANDLE xResult = CreateEvent(nullptr, FALSE, FALSE, CHALLENGE_EVENT);
	if (xResult == nullptr)
	{
		printf("Main: Failed to create event (%d).\n", GetLastError());
		WaitKeyPress(2);
		return S_FALSE;
	}
	
	CustomProcess xProcesses[CHALLENGE_PROCESS_COUNT];
	xProcesses[CHALLENGE_PROCESS_WRITER].Initialise(L"Writer", xMainJobHandle);
	xProcesses[CHALLENGE_PROCESS_FIRSTREADER].Initialise(L"FirstReader", xMainJobHandle);
	/*
	PROCESS_INFORMATION xFirstReaderProcessInfo = {};

	//first process
	{
		PROCESS_INFORMATION xWriterProcessInfo = {};
		//add a process that writes data to the shared buffer
		STARTUPINFO xStartupInfo = {};
		
		xStartupInfo.cb = sizeof(STARTUPINFO);
		TCHAR lpszClientPath[32] = TEXT("Writer.exe");
		if (!CreateProcess(nullptr, lpszClientPath, nullptr, nullptr, FALSE,
			NORMAL_PRIORITY_CLASS | CREATE_NEW_CONSOLE | CREATE_UNICODE_ENVIRONMENT,
			nullptr, nullptr, &xStartupInfo, &xWriterProcessInfo))
		{
			printf("Create Process Failed \n");
			CloseHandle(xMainJobHandle);
			return -1;
		}
	}
	
	if (!AssignProcessToJobObject(xMainJobHandle, xProcessInfo.hProcess))
	{
		printf("AssignProcessToJobObject Failed \n");
	}

	//WaitForSingleObject(xProcessInfo.hProcess, INFINITE);
	*/



	printf("processes launched; waiting for key press \n");
	WaitKeyPress(2);

	UINT32 uExitCode = 0;
	BOOL bResult = TerminateJobObject(xMainJobHandle, uExitCode);
	
	for (u_int u = 0; u < CHALLENGE_PROCESS_COUNT; ++u)
	{
		xProcesses[u].Close();
		//CloseHandle(xProcessInfo.hProcess);
	}

	CloseHandle(hMapFile);
	CloseHandle(xMainJobHandle);

	return uExitCode;
}