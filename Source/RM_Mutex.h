#ifndef CHALLENGE_RM_MUTEX
#define CHALLENGE_RM_MUTEX

#include <windows.h>

class RM_PlatformMutex
{
public:
	RM_PlatformMutex()
	{
		InitializeCriticalSection(&m_xMutex);
	}

	~RM_PlatformMutex()
	{
		DeleteCriticalSection(&m_xMutex);
	}

	void Lock()
	{
		EnterCriticalSection(&m_xMutex);
	}
	void Unlock()
	{
		LeaveCriticalSection(&m_xMutex);
	}

private:
	CRITICAL_SECTION m_xMutex;
};

#endif//CHALLENGE_RM_MUTEX