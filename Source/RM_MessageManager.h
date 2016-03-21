#ifndef CHALLENGE_RM_MESSAGE_MANAGER
#define CHALLENGE_RM_MESSAGE_MANAGER

#include "Utils.h"
#include "RM_SharedMemory.h"
#include "RM_Event.h"

#define MESSAGE_SHARED_MEMORY_NAME "MemoryForMessages"

struct ProcessToken
{
	std::atomic<int> m_iToken;
};

template<int NUM_PROCESSES>
class RM_MessageManagerSharedMemLayout
{
public:
	void Initialise(void* pMemory)
	{
		char* pData = static_cast<char*>(pMemory);
		for (u_int u = 0; u < NUM_PROCESSES; ++u)
		{
			m_xProcessTokens[u] = reinterpret_cast<ProcessToken*>(pData);
			pData += sizeof(ProcessToken);
		}
	}

	void SetToken(int iProcessIndex, int iToken)
	{
		m_xProcessTokens[iProcessIndex]->m_iToken = iToken;
	}

	int GetToken(int iProcessIndex)
	{
		return m_xProcessTokens[iProcessIndex]->m_iToken;
	}

private:
	ProcessToken* m_xProcessTokens[NUM_PROCESSES];
};

template<int NUM_PROCESSES>
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

		for (u_int u = 0; u < NUM_PROCESSES; ++u)
		{
			m_xEvents[u].CreateNamedEvent(u);
		}
	}

	void Initialise(u_int iPID)
	{
		void* pSharedMemory = m_xSharedMemory.OpenMemory(RM_ACCESS_WRITE | RM_ACCESS_READ, MESSAGE_SHARED_MEMORY_MAX_SIZE, 
			TEXT(MESSAGE_SHARED_MEMORY_NAME), iPID);
		m_xSharedMemoryLayout.Initialise(pSharedMemory);

		for (u_int u = 0; u < NUM_PROCESSES; ++u)
		{
			m_xEvents[u].OpenNamedEvent(u);
		}
	}

	void SendMessage(int iToProcessIndex, int iFromProcessIndex, int iToken)
	{
		//TODO: index from
		if (iToProcessIndex == -1)
		{
			//send to all
			for (int i = 0; i < NUM_PROCESSES; ++i)
			{
				if (i != iFromProcessIndex)
				{
					m_xSharedMemoryLayout.SetToken(i, iToken);
					m_xEvents[i].SetEvent();
				}
			}
		}
		else
		{
			m_xSharedMemoryLayout.SetToken(iToProcessIndex, iToken);
			m_xEvents[iToProcessIndex].SetEvent();
		}
	}

	int BlockingReceive(int iProcessIndex)
	{
		//TODO: the process ID does not help here. I need to know who is who in each process
		m_xEvents[iProcessIndex].BlockingWait();
		return m_xSharedMemoryLayout.GetToken(iProcessIndex);
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
	RM_MessageManagerSharedMemLayout<NUM_PROCESSES> m_xSharedMemoryLayout;
	RM_SharedMemory m_xSharedMemory;
};

#endif//CHALLENGE_RM_MESSAGE_MANAGER