#ifndef CHALLENGE_RM_MESSAGE_MANAGER
#define CHALLENGE_RM_MESSAGE_MANAGER

#include "Utils.h"
#include "RM_SharedMemory.h"
#include "RM_Event.h"
#include "RM_Mutex.h"

#define MESSAGE_CHANNELS1_SHARED_MEMORY_NAME "MessageQueue0"
#define MESSAGE_CHANNELS2_SHARED_MEMORY_NAME "MessageQueue1"

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
	RM_LinearLimitedQueue() : m_uIsEmpty(1),m_uIndexFirst(0), m_uIndexLast(0)
	{}

	//not thread safe. Copies the contents of pData if there is enough space
	RM_RETURN_CODE PushBack(const TData* pData)
	{
		//u_int uCurrentIndex = m_uIndexLast;
		
		if (m_uIsEmpty == 0 && m_uIndexLast == m_uIndexFirst)
		{
			//if it is not empty it means we have reached the end of the queue - out of space in the queue
			return RM_CUSTOM_ERR1;
		}
		m_pElements[m_uIndexLast] = *pData;
		m_uIndexLast = (m_uIndexLast + 1) % uNUM_ELEMENTS_LIMIT;
		m_uIsEmpty = 0;
		return RM_SUCCESS;
	}

	RM_RETURN_CODE PopFront(TData* pData_Out)
	{
		if ( m_uIsEmpty == 1)
		//if (m_uIndexFirst == m_uIndexLast)
		{
			//no elements in the queue
			return RM_CUSTOM_ERR1;
		}
		//copy the data and increase the index for the first element
		*pData_Out = m_pElements[m_uIndexFirst];
		m_uIndexFirst = (m_uIndexFirst + 1) % uNUM_ELEMENTS_LIMIT;
		if (m_uIndexFirst == m_uIndexLast) { m_uIsEmpty = 1; }
		return RM_SUCCESS;
	}

private:

	u_int m_uIsEmpty;
	u_int m_uIndexFirst;
	u_int m_uIndexLast;
	TData m_pElements[uNUM_ELEMENTS_LIMIT];
};

template<int NUM_PROCESSES, typename MessageData>
class RM_MessageManagerSharedMemLayout
{
public:

	//intended to be used by processes who are mapping a previously created (and initialised) shared memory
	void MapMemory(void* pMemory)
	{
		char* pData = static_cast<char*>(pMemory);

		//the layout of this shared memory is:
		//[MutexQ0][max num el in queue * size of el for process1][MutexQ1][max num el in queue * size of el for process2] ...[MutexQN][max num el in queue * size of el for processN] 
		for (u_int u = 0; u < NUM_PROCESSES; ++u)
		{
			m_apxMutexes[u] = reinterpret_cast<RM_PlatformMutex*>(pData);
			pData += sizeof(RM_PlatformMutex);

			//this doesn't call the constructor of the queue. It is just a blind reinterpret cast. 
			m_pxMessageQueues[u] = reinterpret_cast<RM_LinearLimitedQueue<MAX_NUM_ELEMENTS_IN_QUEUE, MessageData>*>(pData);
			//m_xProcessTokens[u] = reinterpret_cast<ProcessToken*>(pData);
			pData += sizeof(RM_LinearLimitedQueue<MAX_NUM_ELEMENTS_IN_QUEUE, MessageData>);
		}
	}

	void Create(void* pMemory)
	{
		MapMemory(pMemory);

		//initialise the memory by calling the constructors of the queues
		for (u_int u = 0; u < NUM_PROCESSES; ++u)
		{
			//placement new to call the constructor - very important for locks as they init stuff there
			void* pMem = m_apxMutexes[u];
			new (pMem) RM_PlatformMutex;

			pMem = m_pxMessageQueues[u];
			//placement new will call the constructor for the queue for the memory in pMem - which has been mapped to the shared memory
			//operator new (RM_LinearLimitedQueue<MAX_NUM_ELEMENTS_IN_QUEUE, MessageData>,pMem)
			new (pMem) RM_LinearLimitedQueue<MAX_NUM_ELEMENTS_IN_QUEUE, MessageData>;
		}
	}

	RM_RETURN_CODE PushMessage(int iProcessIndex, const MessageData* pData)
	{
		if (iProcessIndex >= NUM_PROCESSES || iProcessIndex < 0)
		{
			return RM_CUSTOM_ERR1;
		}
		m_apxMutexes[iProcessIndex]->Lock();
		//this is doing a copy of *pData
		m_pxMessageQueues[iProcessIndex]->PushBack(pData);
		m_apxMutexes[iProcessIndex]->Unlock();
		return RM_SUCCESS;
	}

	RM_RETURN_CODE ReadMessage(int iProcessIndex, MessageData* pData_Out)
	{
		if (iProcessIndex >= NUM_PROCESSES || iProcessIndex < 0)
		{
			return RM_CUSTOM_ERR1;
		}
		RM_RETURN_CODE xResult = RM_CUSTOM_ERR1;
		m_apxMutexes[iProcessIndex]->Lock();
		xResult = m_pxMessageQueues[iProcessIndex]->PopFront(pData_Out);
		m_apxMutexes[iProcessIndex]->Unlock();
		return xResult;
	}

	//puts in pData_Out the last message from the queue
	RM_RETURN_CODE FlushQueue(int iProcessIndex, MessageData* pData_Out)
	{
		if (iProcessIndex >= NUM_PROCESSES || iProcessIndex < 0)
		{
			return RM_CUSTOM_ERR1;
		}

		RM_RETURN_CODE xResult = RM_SUCCESS;
		bool bFound = false;
		m_apxMutexes[iProcessIndex]->Lock();
		while (xResult == RM_SUCCESS)
		{
			xResult = m_pxMessageQueues[iProcessIndex]->PopFront(pData_Out);
			bFound = true;
		}
		m_apxMutexes[iProcessIndex]->Unlock();
		if (bFound) { return RM_SUCCESS; }
		return xResult;
	}

private:
	enum
	{
		MAX_NUM_ELEMENTS_IN_QUEUE = 64u,	//to be decided by the app, I should expose this parameter;
	};
	RM_LinearLimitedQueue<MAX_NUM_ELEMENTS_IN_QUEUE,MessageData>* m_pxMessageQueues[NUM_PROCESSES];
	RM_PlatformMutex* m_apxMutexes[NUM_PROCESSES];

public:
	enum
	{
		MAX_MEM_SIZE = (sizeof(RM_LinearLimitedQueue<MAX_NUM_ELEMENTS_IN_QUEUE, MessageData>) + sizeof(RM_PlatformMutex)) * NUM_PROCESSES,
	};
};

template<int NUM_PROCESSES, typename MessageData>
class RM_MessageManager
{
public:
	void Create(u_int iPID, std::string& xObjectName)
	{
		m_xSharedMemory.Create(RM_ACCESS_READ | RM_ACCESS_WRITE, MESSAGE_MANAGER_MEM_SIZE, xObjectName, iPID);
		void* pSharedMemory = m_xSharedMemory.OpenMemory(RM_ACCESS_WRITE | RM_ACCESS_READ, MESSAGE_MANAGER_MEM_SIZE, xObjectName, iPID);
		m_xSharedMemoryLayout.Create(pSharedMemory);

		for (u_int u = 0; u < NUM_PROCESSES; ++u)
		{
			m_xEvents[u].CreateNamedEvent(u, xObjectName.c_str());
		}
	}

	void Initialise(u_int iPID, std::string& xObjectName)
	{
		void* pSharedMemory = m_xSharedMemory.OpenMemory(RM_ACCESS_WRITE | RM_ACCESS_READ, MESSAGE_MANAGER_MEM_SIZE, xObjectName, iPID);
		m_xSharedMemoryLayout.MapMemory(pSharedMemory);

		for (u_int u = 0; u < NUM_PROCESSES; ++u)
		{
			m_xEvents[u].OpenNamedEvent(u, xObjectName.c_str());
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
	}

	RM_RETURN_CODE BlockingReceiveWithFlush(int iProcessIndex, MessageData* pData_Out)
	{
		//TODO: the process ID does not help here. I need to know who is who in each process
		m_xEvents[iProcessIndex].BlockingWait();
		return m_xSharedMemoryLayout.FlushQueue(iProcessIndex, pData_Out);
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
		//how much memory each queue occupies. Given by the layout class.
		MESSAGE_MANAGER_MEM_SIZE = RM_MessageManagerSharedMemLayout<NUM_PROCESSES, RM_WToRMessageData>::MAX_MEM_SIZE
	};

	RM_Event m_xEvents[NUM_PROCESSES];
	RM_MessageManagerSharedMemLayout<NUM_PROCESSES, RM_WToRMessageData> m_xSharedMemoryLayout;
	RM_SharedMemory m_xSharedMemory;
};

#endif//CHALLENGE_RM_MESSAGE_MANAGER