#ifndef CHALLENGE_RM_SHARED_MEMORY
#define CHALLENGE_RM_SHARED_MEMORY

#include "Win10/Platform_Utils.h"

class RM_SharedMemory
{
public:
	//flags can be RM_ACCESS_FLAG
	RM_RETURN_CODE Create(u_int uFlags, long long llSharedMemMaxSize, std::string& xObjectName, u_int uPID)
	{
		return m_xPlatformMem.CreateMemory(uFlags, llSharedMemMaxSize, xObjectName, uPID);
	}

	void* OpenMemory(u_int uFlags, long long llSharedMemMaxSize, std::string& xObjectName, u_int uPID)
	{
		return m_xPlatformMem.OpenMemory(uFlags, llSharedMemMaxSize, xObjectName, uPID);
	}

	void CloseMemory()
	{
		m_xPlatformMem.CloseMemory();
	}

private:
	Platform_SharedMemory m_xPlatformMem;
};

#endif//CHALLENGE_RM_SHARED_MEMORY