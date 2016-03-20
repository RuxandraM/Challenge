#ifndef CHALLENGE_RM_PLATFORM_UTILS
#define CHALLENGE_RM_PLATFORM_UTILS

#include <windows.h>
#include <process.h>
#include <atomic>
#include <iostream>
#include "../Utils.h"

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

class Platform_SharedMemory
{
public:
	Platform_SharedMemory() :m_pMemory(nullptr), m_hMapFile(nullptr) {}
	~Platform_SharedMemory()
	{
		CloseMemory();
	}

	RM_RETURN_CODE CreateMemory(u_int uFlags, long long llSharedMemMaxSize, TCHAR szName[], u_int uPID)
	{
		//create shared memory
		m_hMapFile = CreateFileMapping(
			INVALID_HANDLE_VALUE,    // invalid means to share just memory, not a file
			NULL,                    // default security
			PAGE_READWRITE,          // read/write access
			0,                       // maximum object size (high-order DWORD)
			(DWORD)llSharedMemMaxSize,		 // maximum object size (low-order DWORD)
			szName);                 // name of mapping object

		if (m_hMapFile == nullptr)
		{
			printf("Main: Failed to create file mapping object (%d).\n", GetLastError());
			WaitKeyPress(2);
			return RM_CUSTOM_ERR1;
		}
		return RM_SUCCESS;
	}

	//flags can be RM_ACCESS_FLAG
	const void* OpenMemory(u_int uFlags, long long llSharedMemMaxSize, TCHAR szName[], u_int uPID)
	{
		DWORD xPlatformAccessFlag = 0;
		if ((uFlags & RM_ACCESS_READ) &&
			(uFlags & RM_ACCESS_WRITE))
		{
			xPlatformAccessFlag = FILE_MAP_ALL_ACCESS;
		}
		else if (uFlags & RM_ACCESS_READ)
		{
			xPlatformAccessFlag = FILE_MAP_READ;
		}
		else if (uFlags & RM_ACCESS_WRITE)
		{
			xPlatformAccessFlag = FILE_MAP_WRITE;
		}
		else return nullptr;

		//long long llSharedMemMaxSize = SHARED_MEMORY_MAX_SIZE;
		//TCHAR szName[] = TEXT(SHARED_MEMORY_NAME);
		m_hMapFile = OpenFileMapping(
			xPlatformAccessFlag,   // read/write access
			FALSE,                 // do not inherit the name
			szName);               // name of mapping object

		if (m_hMapFile == nullptr)
		{
			printf("[%d] Writer: Failed to open file mapping object (%d).\n", uPID, GetLastError());
			//WaitKeyPress(1);
			return nullptr;
		}

		const WCHAR* pSharedMemory = (LPTSTR)MapViewOfFile(m_hMapFile,   // handle to map object
			xPlatformAccessFlag, // read/write permission
			0,
			0,
			llSharedMemMaxSize);

		if (pSharedMemory == nullptr)
		{
			printf("[%d] Writer: Failed to map shared memory! Quitting. \n", uPID);
			CloseHandle(m_hMapFile);
			//WaitKeyPress(1);
			return nullptr;
		}

		return pSharedMemory;
	}

	void CloseMemory()
	{
		if (m_pMemory)
		{
			UnmapViewOfFile(m_pMemory);
			m_pMemory = nullptr;
		}

		if (m_hMapFile)
		{
			CloseHandle(m_hMapFile);
			m_hMapFile = nullptr;
		}
	}

private:
	const WCHAR* m_pMemory;
	HANDLE m_hMapFile;
};

#endif //CHALLENGE_RM_PLATFORM_UTILS