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

class RM_ThreadPool
{
public:
	void Initialise(u_int uNumThreads, ThreadSharedParamGroup& xSharedParamsGroup)
	{
		//m_xSleepingThreads.reserve(uNumThreads);
		//m_xActiveThreads.reserve(uNumThreads);
		//m_xWakeUpEvents.reserve(uNumThreads);

		m_xSleepingPoolMutex.Lock();
		for (u_int u = 0; u < uNumThreads; ++u)
		{
			RM_EventThread* pOutputThread = new RM_SR_OutputThread(xSharedParamsGroup);
			m_xSleepingThreads.push_back(pOutputThread);
		}
		m_xSleepingPoolMutex.Unlock();

		//StartThreads();
	}

	void StartThreads()
	{
		for (ThreadListIt it = m_xSleepingThreads.begin(); it != m_xSleepingThreads.end(); ++it)
		{
			(*it)->Start();
		}
	}

	//void StartThreads(void* pContext)
	//{
	//	for (ThreadListIt it = m_xSleepingThreads.begin(); it != m_xSleepingThreads.end(); ++it)
	//	{
	//		RM_EventThread* pThread = *it;
	//		pThread->ResetContext(pContext);
	//		//launches the threads, putting them in the sleep state
	//		pThread->Start();
	//	}
	//}

	void Shutdown()
	{
		//terminate all the threads
		m_xSleepingPoolMutex.Lock();
		for (ThreadListIt it = m_xSleepingThreads.begin(); it != m_xSleepingThreads.end(); ++it)
		{
			RM_EventThread* pCurrentThread = *it;
			pCurrentThread->StartShutdown();
		}
		m_xSleepingPoolMutex.Unlock();

		m_xActivePoolMutex.Lock();
		for (ThreadListIt it = m_xActiveThreads.begin(); it != m_xActiveThreads.end(); ++it)
		{
			RM_EventThread* pCurrentThread = *it;
			pCurrentThread->StartShutdown();
		}
		m_xActivePoolMutex.Unlock();
	}

	void ChangeThreadToSleeping(RM_EventThread* pCurrent)
	{
		m_xActivePoolMutex.Lock();
		m_xActiveThreads.remove(pCurrent);
		m_xActivePoolMutex.Unlock();

		m_xSleepingPoolMutex.Lock();
		m_xSleepingThreads.push_back(pCurrent);
		m_xSleepingPoolMutex.Unlock();
	}

	RM_EventThread* GetThreadForActivation(ThreadSharedParamGroup& xSharedParamsGroup)
	{
		RM_EventThread* pResultThread = nullptr;
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
			pResultThread = new RM_SR_OutputThread(xSharedParamsGroup);
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
	std::list<RM_EventThread*> m_xSleepingThreads;
	std::list<RM_EventThread*> m_xActiveThreads;
	typedef std::list<RM_EventThread*>::iterator ThreadListIt;
};

#endif//CHALLENGE_RM_THREAD_POOL