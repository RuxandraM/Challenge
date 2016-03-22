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


//GUGU - to delete?!
//TODO: this is a normal lock, but the desired functionality is - 
//a writer locks everything, multiple readers can read in the same time
class RM_ReadWriteMutex
{
public:

	void LockForWriting()
	{
		m_xMutex.Lock();
	}
	void UnlockFromWrite()
	{
		m_xMutex.Lock();
	}

	void LockForReading()
	{
		m_xMutex.Lock();
	}
	void UnlockFromRead()
	{
		m_xMutex.Unlock();
	}

private:
	RM_PlatformMutex m_xMutex;
};

#endif//CHALLENGE_RM_MUTEX