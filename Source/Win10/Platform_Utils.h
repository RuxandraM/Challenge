#ifndef CHALLENGE_RM_PLATFORM_UTILS
#define CHALLENGE_RM_PLATFORM_UTILS

#include <windows.h>
#include <process.h>
#include <atomic>
#include <iostream>
#include "../Utils.h"

static void WaitKeyPress(int iKey)
{
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

	RM_RETURN_CODE CreateMemory(u_int uFlags, long long llSharedMemMaxSize, std::string& xObjectName, u_int uPID)
	{
		std::wstring wszName(xObjectName.c_str(), xObjectName.c_str() + strlen(xObjectName.c_str()));

		//create shared memory
		m_hMapFile = CreateFileMapping(
			INVALID_HANDLE_VALUE,    // invalid means to share just memory, not a file
			NULL,                    // default security
			PAGE_READWRITE,          // read/write access
			0,                       // maximum object size (high-order DWORD)
			(DWORD)llSharedMemMaxSize,		 // maximum object size (low-order DWORD)
			wszName.c_str());                 // name of mapping object

		if (m_hMapFile == nullptr)
		{
			printf("Main: Failed to create file mapping object %s (%d).\n", xObjectName.c_str(), GetLastError());
			return RM_CUSTOM_ERR1;
		}
		
		return RM_SUCCESS;
	}

	//flags can be RM_ACCESS_FLAG
	void* OpenMemory(u_int uFlags, long long llSharedMemMaxSize, std::string& xObjectName, u_int uPID)
	{
		std::wstring wszName(xObjectName.c_str(), xObjectName.c_str() + strlen(xObjectName.c_str()));

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

		m_hMapFile = OpenFileMapping(
			xPlatformAccessFlag,   // read/write access
			FALSE,                 // do not inherit the name
			wszName.c_str());      // name of mapping object

		if (m_hMapFile == nullptr)
		{
			printf("[%d] Failed to map memory for object %s err:%d.\n", uPID, xObjectName.c_str(), GetLastError());
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
			return nullptr;
		}

		return (void*)pSharedMemory;
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