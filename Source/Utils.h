#ifndef NANOPORE_CHALLENGE_UTILS
#define NANOPORE_CHALLENGE_UTILS

#include <windows.h>
#include <process.h>
#include <atomic>

typedef unsigned int u_int;

struct SharedLabels
{
	std::atomic<int> m_iTimestamp;
	std::atomic<int> m_iWriteIndex;	//current segment to write
};

struct SegmentInfo
{
	std::atomic<int> m_iWriteTimestamp;
	std::atomic<int> m_iIsInUseForWriting;
	std::atomic<int> m_iIsInUseForReading;
	std::atomic<int> m_iWriteRequestSet;
	//volatile int m_iIsInUseForWriting;
	//volatile int m_iIsInUseForReading;
	//volatile int m_iWriteRequestSet;
};

#define SEGMENT_SIZE 1000000
#define NUM_SEGMENTS 10

#define SHARED_MEMORY_MAX_SIZE (sizeof(SharedLabels) + NUM_SEGMENTS * (sizeof(SegmentInfo) + SEGMENT_SIZE))
#define SHARED_MEMORY_NAME "NanoporeChallengeSharedMemory"
#define CHALLENGE_EVENT L"ChallengeEvent"


#include <iostream>
static void WaitKeyPress(int iKey)
{
	//system("pause");
	std::cin.clear();
	int iInput = 0;
	while (iInput != iKey)
	{
		std::cin >> iInput;
	}
}


class CustomProcess
{
public:
	HRESULT Initialise(const WCHAR* szExecutable, HANDLE xMainJobHandle)
	{
		u_int uLen = static_cast<u_int>(wcslen(szExecutable));
		if (uLen >= 32) return S_FALSE;

		//add a process that writes data to the shared buffer
		STARTUPINFO xStartupInfo = {};

		xStartupInfo.cb = sizeof(STARTUPINFO);

		TCHAR lpszClientPath[32];
		memcpy(lpszClientPath, szExecutable, uLen * sizeof(WCHAR));
		lpszClientPath[uLen] = '\0';

		DWORD xFlags = NORMAL_PRIORITY_CLASS | CREATE_UNICODE_ENVIRONMENT;
		if (!xMainJobHandle)
		{
			xFlags |= CREATE_NEW_PROCESS_GROUP;
		}

		if (!CreateProcess(nullptr, lpszClientPath, nullptr, nullptr, FALSE,
			NORMAL_PRIORITY_CLASS | CREATE_NEW_CONSOLE | CREATE_UNICODE_ENVIRONMENT,
			//xFlags,
			nullptr, nullptr, &xStartupInfo, &m_xProcessInfo))
		{
			printf("Create Process %Ls Failed \n", szExecutable);
			return S_FALSE;
		}

		if (!xMainJobHandle)
		{
			return S_OK;
		}

		if (!AssignProcessToJobObject(xMainJobHandle, m_xProcessInfo.hProcess))
		{
			printf("AssignProcessToJobObject Failed \n");
			return S_FALSE;
		}

		return S_OK;
	}

	void Close()
	{
		CloseHandle(m_xProcessInfo.hProcess);
	}

private:

	PROCESS_INFORMATION m_xProcessInfo;
};
#endif//NANOPORE_CHALLENGE_UTILS