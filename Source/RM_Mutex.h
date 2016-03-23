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

//a thread safe class that can let processes know if a segment is 
//used for read/write, without locking the actual data - it is the 
//process' responsibility to sync access to their shared objects
class RM_ReadWriteLogic
{
public:
	RM_ReadWriteLogic() :m_bInUseForWriting(false), m_uNumActiveReaders(0){}

	bool AddWriteRequest()
	{
		bool bResult = false;
		m_xLockFlags.Lock();
		if (!m_bInUseForWriting && m_uNumActiveReaders == 0)
		{
			bResult = true;
			m_bInUseForWriting = true;
		}
		m_xLockFlags.Unlock();
		return bResult;
	}

	bool AddReadRequest()
	{
		bool bResult = false;
		m_xLockFlags.Lock();
		if (!m_bInUseForWriting)
		{
			bResult = true;
			++m_uNumActiveReaders;
		}
		m_xLockFlags.Unlock();
		return bResult;
	}

	void SetFinishedWriting()
	{
		m_xLockFlags.Lock();
		m_bInUseForWriting = false;
		m_xLockFlags.Unlock();
	}

	void SetFinishedReading()
	{
		m_xLockFlags.Lock();
		--m_uNumActiveReaders;
		m_xLockFlags.Unlock();
	}

	void Reset()
	{
		m_bInUseForWriting = false;
		m_uNumActiveReaders = 0;
	}

private:
	RM_PlatformMutex m_xLockFlags;
	bool m_bInUseForWriting;
	u_int m_uNumActiveReaders;
};

class RM_ReadWriteLock
{
public:
	RM_ReadWriteLock() :m_bInUseForWriting(false), m_uNumActiveReaders(0)
	{}

	bool LockForReading()
	{
		bool bWasReading = false;
		{
			m_xLockFlags.Lock();
			bWasReading = (m_uNumActiveReaders != 0);
			++m_uNumActiveReaders;
			m_xLockFlags.Unlock();
		}
		if (!bWasReading){ m_xLockData.Lock(); }
		return bWasReading;
	}

	bool UnlockForReading()
	{
		bool bUnlockedMutex = false;
		m_xLockFlags.Lock();
		--m_uNumActiveReaders;
		if (m_uNumActiveReaders == 0)
		{
			bUnlockedMutex = true;
			m_xLockData.Unlock();
		}
		m_xLockFlags.Unlock();
		return bUnlockedMutex;
	}

	bool LockForWriting()
	{
		bool bWillWait = false;
		{
			m_xLockFlags.Lock();
			if ((m_uNumActiveReaders == 0) || m_bInUseForWriting) { bWillWait = true; }
			m_bInUseForWriting = true;
			m_xLockFlags.Unlock();
		}
		m_xLockData.Lock();
		return bWillWait;
	}

	bool UnlockFromWriting()
	{
		{
			m_xLockFlags.Lock();
			//if (!m_bInUseForWriting) error
			m_bInUseForWriting = true;
			m_xLockFlags.Unlock();
		}
		m_xLockData.Unlock();
	}

private:
	RM_PlatformMutex m_xLockFlags;
	RM_PlatformMutex m_xLockData;
	bool m_bInUseForWriting;
	bool m_bInUseForReading;
	u_int m_uNumActiveReaders;
};

#endif//CHALLENGE_RM_MUTEX