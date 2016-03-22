#ifndef CHALLENGE_RM_MESSAGE_MANAGER
#define CHALLENGE_RM_MESSAGE_MANAGER

#include "Utils.h"
#include "RM_SharedMemory.h"
#include "RM_Event.h"
#include "RM_Mutex.h"

#define MESSAGE_SHARED_MEMORY_NAME "MemoryForMessages"

struct ProcessToken
{
	std::atomic<int> m_iToken;
};

//a queue that does not allocate memory - I needed this to map it onto the shared memory;
//it is limited to be able to stack a few queues one after the other in the same block of contiguous memory.
//It is intended to be used as a message queue in between processes. It is not thread safe, it is the 
//message manager responsibility to synchronise the access to this
template< u_int uNUM_ELEMENTS_LIMIT, typename TData >
class RM_LinearLimitedQueue
{
public:
	RM_LinearLimitedQueue() : m_uIndexFirst(0), m_uIndexLast(0)
	{}

	//not thread safe. Copies the contents of pData if there is enough space
	RM_RETURN_CODE PushBack(const TData* pData)
	{
		m_uIndexLast = (m_uIndexLast + 1) % uNUM_ELEMENTS_LIMIT;
		if (m_uIndexLast == m_uIndexFirst)
		{
			//out of space in the queue
			return RM_CUSTOM_ERR1;
		}
		m_pElements[m_uIndexLast] = *pData;
		return RM_SUCCESS;
	}

	RM_RETURN_CODE PopFront(TData* pData_Out)
	{
		if (m_uIndexFirst == m_uIndexLast)
		{
			//no elements in the queue
			return RM_CUSTOM_ERR1;
		}
		//copy the data and increase the index for the first element
		*pData_Out = m_pElements[m_uIndexFirst];
		m_uIndexFirst = (m_uIndexFirst + 1) % uNUM_ELEMENTS_LIMIT;
		return RM_SUCCESS;
	}

private:
	u_int m_uIndexFirst;
	u_int m_uIndexLast;
	TData m_pElements[uNUM_ELEMENTS_LIMIT];
};

template<int NUM_PROCESSES, typename MessageData>
class RM_MessageManagerSharedMemLayout
{
public:

	//intended to be used by processes who are mapping a previously created (and initialised) shared memory
	void Map(void* pMemory)
	{
		char* pData = static_cast<char*>(pMemory);
		//the layout of this shared memory is:
		//[max num el in queue * size of el for process1][max num el in queue * size of el for process2] ...[max num el in queue * size of el for processN] 
		for (u_int u = 0; u < NUM_PROCESSES; ++u)
		{
			//this doesn't call the constructor of the queue. It is just a blind reinterpret cast. 
			m_pxMessageQueues[u] = reinterpret_cast<RM_LinearLimitedQueue<MAX_NUM_ELEMENTS_IN_QUEUE, MessageData>*>(pData);
			//m_xProcessTokens[u] = reinterpret_cast<ProcessToken*>(pData);
			pData += sizeof(RM_LinearLimitedQueue<MAX_NUM_ELEMENTS_IN_QUEUE, MessageData>);
		}
	}

	void Create(void* pMemory)
	{
		Map(pMemory);

		//initialise the memory by calling the constructors of the queues
		for (u_int u = 0; u < NUM_PROCESSES; ++u)
		{
			void* pMem = m_pxMessageQueues[u];
			//placement new will call the constructor for the queue for the memory in pMem - which has been mapped to the shared memory
			//operator new (RM_LinearLimitedQueue<MAX_NUM_ELEMENTS_IN_QUEUE, MessageData>,pMem)
			new (pMem) RM_LinearLimitedQueue<MAX_NUM_ELEMENTS_IN_QUEUE, MessageData>;
		}
	}

	//void SetToken(int iProcessIndex, int iToken)
	//{
	//	m_xProcessTokens[iProcessIndex]->m_iToken = iToken;
	//}
	//
	//int GetToken(int iProcessIndex)
	//{
	//	return m_xProcessTokens[iProcessIndex]->m_iToken;
	//}

	RM_RETURN_CODE PushMessage(int iProcessIndex, const MessageData* pData)
	{
		if (iProcessIndex >= NUM_PROCESSES || iProcessIndex < 0)
		{
			return RM_CUSTOM_ERR1;
		}
		m_xMutex[iProcessIndex].Lock();
		//this is doing a copy of *pData
		m_pxMessageQueues[iProcessIndex]->PushBack(pData);
		m_xMutex[iProcessIndex].Unlock();
		return RM_SUCCESS;
	}

	RM_RETURN_CODE ReadMessage(int iProcessIndex, MessageData* pData_Out)
	{
		if (iProcessIndex >= NUM_PROCESSES || iProcessIndex < 0)
		{
			return RM_CUSTOM_ERR1;
		}
		RM_RETURN_CODE xResult = RM_CUSTOM_ERR1;
		m_xMutex[iProcessIndex].Lock();
		xResult = m_pxMessageQueues[iProcessIndex]->PopFront(pData_Out);
		m_xMutex[iProcessIndex].Unlock();
		return xResult;
	}

private:
	//ProcessToken* m_xProcessTokens[NUM_PROCESSES];
	enum
	{
		MAX_NUM_ELEMENTS_IN_QUEUE = 64u,	//to be decided by the app, I should expose this parameter; GUGU
	};
	RM_LinearLimitedQueue<MAX_NUM_ELEMENTS_IN_QUEUE,MessageData>* m_pxMessageQueues[NUM_PROCESSES];
	RM_PlatformMutex m_xMutex[NUM_PROCESSES];
};

template<int NUM_PROCESSES, typename MessageData>
class RM_MessageManager
{
public:
	void Create(u_int iPID)
	{
		for (u_int u = 0; u < NUM_PROCESSES; ++u)
		{
			m_xEvents[u].CreateNamedEvent(u);
		}

		m_xSharedMemory.Create(RM_ACCESS_READ | RM_ACCESS_WRITE, MESSAGE_SHARED_MEMORY_MAX_SIZE, TEXT(MESSAGE_SHARED_MEMORY_NAME), iPID);
		void* pSharedMemory = m_xSharedMemory.OpenMemory(RM_ACCESS_WRITE | RM_ACCESS_READ, MESSAGE_SHARED_MEMORY_MAX_SIZE,
			TEXT(MESSAGE_SHARED_MEMORY_NAME), iPID);
		m_xSharedMemoryLayout.Create(pSharedMemory);

		for (u_int u = 0; u < NUM_PROCESSES; ++u)
		{
			m_xEvents[u].CreateNamedEvent(u);
		}
	}

	void Initialise(u_int iPID)
	{
		void* pSharedMemory = m_xSharedMemory.OpenMemory(RM_ACCESS_WRITE | RM_ACCESS_READ, MESSAGE_SHARED_MEMORY_MAX_SIZE, 
			TEXT(MESSAGE_SHARED_MEMORY_NAME), iPID);
		m_xSharedMemoryLayout.Map(pSharedMemory);

		for (u_int u = 0; u < NUM_PROCESSES; ++u)
		{
			m_xEvents[u].OpenNamedEvent(u);
		}
	}

	RM_RETURN_CODE SendMessage(int iToProcessIndex, int iFromProcessIndex, const MessageData* pData)
	{
		//TODO: implement index from
		RM_RETURN_CODE xResult = RM_SUCCESS;
		//indexTo == -1 means we are sending the message to all other processes but myself
		if (iToProcessIndex == -1)
		{
			for (int i = 0; i < NUM_PROCESSES; ++i)
			{
				if (i != iFromProcessIndex)
				{
					//write to the shared memory
					RM_RETURN_CODE xTempResult = m_xSharedMemoryLayout.PushMessage(i, pData);
					if (xTempResult != RM_SUCCESS)
					{
						printf("Failed to send the message. Queue is full? Err code %d \n", xTempResult);
						xResult = xTempResult;	//keep track of failures, but try sending the rest of the messages to other readers
					}
					//signal the corresponding processes to trigger a message read
					m_xEvents[i].SetEvent();
				}
			}
			return xResult;
		}
		else
		{
			xResult = m_xSharedMemoryLayout.PushMessage(iToProcessIndex, pData);
			if (xResult != RM_SUCCESS)
			{
				printf("Failed to send the message. Queue is full? Err code %d \n", xResult);
				return xResult;
			}
			m_xEvents[iToProcessIndex].SetEvent();
		}
		return RM_SUCCESS;
	}

	RM_RETURN_CODE BlockingReceive(int iProcessIndex, MessageData* pData_Out)
	{
		//TODO: the process ID does not help here. I need to know who is who in each process
		m_xEvents[iProcessIndex].BlockingWait();
		return m_xSharedMemoryLayout.ReadMessage(iProcessIndex, pData_Out);
		//return m_xSharedMemoryLayout.GetToken(iProcessIndex);
	}

	void Shutdown()
	{
		for (u_int u = 0; u < NUM_PROCESSES; ++u)
		{
			m_xEvents[u].Close();
		}
	}

private:
	enum
	{
		MESSAGE_SHARED_MEMORY_MAX_SIZE = NUM_PROCESSES * sizeof(ProcessToken),
	};

	RM_Event m_xEvents[NUM_PROCESSES];
	RM_MessageManagerSharedMemLayout<NUM_PROCESSES, RM_WToRMessageData> m_xSharedMemoryLayout;
	RM_SharedMemory m_xSharedMemory;
};

#endif//CHALLENGE_RM_MESSAGE_MANAGER