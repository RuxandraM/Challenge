#ifndef CHALLENGE_RM_THREAD_POOL
#define CHALLENGE_RM_THREAD_POOL

#include "..\Source\Utils.h"
#include "..\Source\SharedBuffer.h"
#include "..\Source\RM_SharedMemory.h"
#include "..\Source\RM_StagingBuffer.h"
#include "..\Source\RM_MessageManager.h"
#include "..\Source\RM_Thread.h"
#include "..\Source\RM_Mutex.h"
#include "SecondReader.h"
#include <list>

//GUGU
#include "RM_OutputThread.h"

struct ThreadSharedParamGroup;
class RM_SR_OutputThread;

//TThread must derive from RM_Thread
template<typename TThread, typename TContext>
class RM_ThreadPool
{
public:
	void Initialise(u_int uNumThreads, TContext* pContext)
	{
		m_xSleepingPoolMutex.Lock();
		for (u_int u = 0; u < uNumThreads; ++u)
		{
			TThread* pOutputThread = new TThread(pContext);
			m_xSleepingThreads.push_back(pOutputThread);
		}
		m_xSleepingPoolMutex.Unlock();
	}

	void StartThreads()
	{
		for (std::list<TThread*>::iterator it = m_xSleepingThreads.begin(); it != m_xSleepingThreads.end(); ++it)
		{
			(*it)->Start();
		}
	}

	void Shutdown()
	{
		//terminate all the threads
		m_xSleepingPoolMutex.Lock();
		for (std::list<TThread*>::iterator it = m_xSleepingThreads.begin(); it != m_xSleepingThreads.end(); ++it)
		{
			(*it)->StartShutdown();
		}
		m_xSleepingPoolMutex.Unlock();

		m_xActivePoolMutex.Lock();
		for (std::list<TThread*>::iterator it = m_xActiveThreads.begin(); it != m_xActiveThreads.end(); ++it)
		{
			(*it)->StartShutdown();
		}
		m_xActivePoolMutex.Unlock();
	}

	void ChangeThreadToSleeping(TThread* pCurrent)
	{
		m_xActivePoolMutex.Lock();
		m_xActiveThreads.remove(pCurrent);
		m_xActivePoolMutex.Unlock();

		m_xSleepingPoolMutex.Lock();
		m_xSleepingThreads.push_back(pCurrent);
		m_xSleepingPoolMutex.Unlock();
	}

	TThread* GetThreadForActivation(TContext* pContext)
	{
		TThread* pResultThread = nullptr;
		m_xSleepingPoolMutex.Lock();
		size_t xSize = m_xSleepingThreads.size();
		if (xSize != 0)
		{
			pResultThread = m_xSleepingThreads.back();
			m_xSleepingThreads.pop_back();
		}
		m_xSleepingPoolMutex.Unlock();

		if (!pResultThread)
		{
			pResultThread = new TThread(pContext);
			pResultThread->Start();
		}

		m_xActivePoolMutex.Lock();
		m_xActiveThreads.push_back(pResultThread);
		m_xActivePoolMutex.Unlock();

		return pResultThread;
	}

private:
	RM_PlatformMutex m_xSleepingPoolMutex;
	RM_PlatformMutex m_xActivePoolMutex;
	//TODO: either resize or change data type
	//this is in danger of allocating memory every time we run out of threads in the sleeping pool
	std::list<TThread*> m_xSleepingThreads;
	std::list<TThread*> m_xActiveThreads;
};

#endif//CHALLENGE_RM_THREAD_POOL