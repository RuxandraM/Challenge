#ifndef CHALLENGE_RM_READER_PROCESS
#define CHALLENGE_RM_READER_PROCESS

#include <stdio.h>
#include <windows.h>
#include "../Source/Utils.h"
#include "../Source/RM_MessageManager.h"
#include "../Source/RM_ReaderOutputThread.h"
#include "../Source/RM_SharedInitData.h"
#include "../Source/RM_SharedBuffer.h"
#include "../Source/RM_SharedMemory.h"
#include "../Source/RM_StagingBuffer.h"
#include "../Source/RM_Thread.h"
#include "../Source/RM_ThreadPool.h"


class RM_ReaderProcess
{
public:
	RM_ReaderProcess(int iProcessIndex) :m_iPID(0), m_iProcessIndex(iProcessIndex){ m_iPID = _getpid(); }
	virtual ~RM_ReaderProcess() {}

	virtual void Initialise()
	{
		m_iPID = _getpid();

		RM_InitData xInitData;
		{
			RM_SharedMemory xSharedMemory;
			std::string xSharedInitDataName(SHARED_INIT_DATA_NAME);
			void* pSharedMemory = xSharedMemory.OpenMemory(RM_ACCESS_WRITE | RM_ACCESS_READ, SHARED_INIT_DATA_MAX_SIZE, xSharedInitDataName, m_iPID);
			RM_SharedInitDataLayout xInitDataLayout;
			xInitDataLayout.MapMemory(pSharedMemory);
			xInitData = xInitDataLayout.GetInitData(m_iProcessIndex);
		}

		RM_SharedMemory xSharedMemory;
		std::string xSharedBuffName(xInitData.pSharedBufferMemNameIn);
		void* pSharedMemory = xSharedMemory.OpenMemory(RM_ACCESS_READ, SHARED_MEMORY_MAX_SIZE, xSharedBuffName, m_iPID);

		RM_SharedMemory xSharedMemoryLabels;
		std::string xLablesObjName(xSharedBuffName);
		xLablesObjName.append("Lables");
		void* pSharedMemoryLabels = xSharedMemoryLabels.OpenMemory(RM_ACCESS_WRITE | RM_ACCESS_READ, SHARED_MEMORY_LABESLS_MAX_SIZE, xLablesObjName, m_iPID);

		std::string xMsgQName(xInitData.pMessageQueueNameIn);
		m_xMessageManager.Initialise(m_iPID, xMsgQName);
		
		m_xSharedBuffer.MapMemory(pSharedMemory, pSharedMemoryLabels);

		m_xGlobalOutputFileName = xInitData.pOutputFileName;

		printf("[%d] Reader initialised. Waiting for the writer to start \n", m_iPID);
	}

	virtual void Shutdown() {}

	virtual RM_RETURN_CODE ReadData() = 0;

protected:
	int m_iPID;
	int m_iProcessIndex;
	RM_SharedBuffer m_xSharedBuffer;
	RM_MessageManager<RM_CHALLENGE_PROCESS_COUNT, RM_WToRMessageData> m_xMessageManager;
	std::string m_xGlobalOutputFileName;
};

template<typename TStagingThread>
class RM_StagingReader : public RM_ReaderProcess
{
public:
	typedef RM_ReaderProcess PARENT;

	RM_StagingReader(int iProcessIndex, u_int uMaxNumRandomSegments, u_int uMaxNumPoolThreads, u_int uNumStagingSegments) :RM_ReaderProcess(iProcessIndex),
		m_uNumGroupSegments(0),
		m_uCurrentGroupIteration(0),
		m_uSegmentIndexInGroup(0),
		m_uMaxNumRandomSegments(uMaxNumRandomSegments),
		m_uMaxNumPoolThreads(uMaxNumPoolThreads),
		m_uNumStagingSegments(uNumStagingSegments)
	{
		
	}
	virtual ~RM_StagingReader() {}

	virtual void Initialise()
	{
		PARENT::Initialise();

		m_xThreadSharedParamGroup.m_pSharedBuffer = &m_xSharedBuffer;
		m_xThreadSharedParamGroup.m_pStagingBuffer = &m_xStagingBuffer;
		m_xThreadSharedParamGroup.m_pThreadPool = &m_xThreadPool;

		m_xThreadPool.Initialise(m_uMaxNumPoolThreads, &m_xThreadSharedParamGroup);
		m_xStagingBuffer.Initialise(m_uNumStagingSegments);

		m_xThreadPool.StartThreads();
	}

	RM_RETURN_CODE StartNewGroup()
	{
		m_uNumGroupSegments = 1 + std::rand() % m_uMaxNumRandomSegments;
		m_uSegmentIndexInGroup = 0;
		return CustomReaderStartNewGroup();
	}

	virtual void ResetThreadContext(TStagingThread* pStagingThread, typename TStagingThread::StagingContext& xStagingContext) = 0;
	virtual void CloseThreadContext() = 0;
	virtual RM_RETURN_CODE CustomReaderStartNewGroup() = 0;
	virtual void EndIteration() = 0;

	RM_RETURN_CODE ReadData()
	{
		//start a new iteration. This will generate the number of segments in the group, and will reset the counters
		RM_RETURN_CODE xResult = StartNewGroup();
		if (xResult != RM_SUCCESS) return xResult;
		
		//keep listening
		while (true)
		{

			RM_WToRMessageData xMessage;
			RM_RETURN_CODE xResult = m_xMessageManager.BlockingReceive(m_iProcessIndex, &xMessage);
			if (xResult != RM_SUCCESS)
			{
				printf("[%d] Failed to read signalled message. \n", m_iPID);
				continue;	//go back and wait for other messages. It was a problem in the transmition medium
			}

			int iStagingSegmentIndex = m_xStagingBuffer.ReserveSegmentIfAvailable();
			if (iStagingSegmentIndex == -1)
			{
				printf("[%d] Out of staging buffer space \n", m_iPID);
				return RM_CUSTOM_ERR2;
			}
			//this will always return me a thread, it will create one if needed; if creation fails it means that something wrong has happened
			TStagingThread* pOutputThread = m_xThreadPool.GetThreadForActivation(&m_xThreadSharedParamGroup);
			if (!pOutputThread)
			{
				printf("No available threads \n");
				return RM_CUSTOM_ERR1;
			}
			if (iStagingSegmentIndex != -1)
			{
				TStagingThread::StagingContext xStagingContext(xMessage.m_uTag, xMessage.m_uSegmentWritten, iStagingSegmentIndex);
				ResetThreadContext(pOutputThread, xStagingContext);
				
				RM_Event* pxEvent = pOutputThread->GetEvent();
				pxEvent->SetEvent();

				CloseThreadContext();
			}
			else
			{
				//TODO: I could try reading straight from the shared buffer, hoping that the circuar character of the buffer will allow me
				//to read without interfering with the writer.
				printf("[%d] Out of staging buffer space. I will skip reading this segment.\n", m_iPID);
				return RM_CUSTOM_ERR1;
			}

			//count the consecutive segments since the start of this group
			++m_uSegmentIndexInGroup;
			
			//custom logic in the derived classes to let them know we progressed to the next segment and we need to prepare for the next iteration
			EndIteration();
			

			//----------------------------------------------------------------------
			//Note: a thread has the file and it will need to close it. I will not wait here to close, just delete the pointers
			//TODO: store a list of files that need closing to handle error cases.
			if (m_uSegmentIndexInGroup == m_uNumGroupSegments)
			{
				//prepare for the next group iteration
				++m_uCurrentGroupIteration;
				StartNewGroup();
			}
		}
	}

	void Shutdown()
	{
		PARENT::Shutdown();
		m_xThreadPool.Shutdown();
	}

protected:
	u_int m_uNumGroupSegments;
	u_int m_uCurrentGroupIteration;
	u_int m_uSegmentIndexInGroup;

private:	
	
	u_int m_uCurrentFile;
	
	u_int m_uNumStagingSegments;
	RM_StagingBuffer m_xStagingBuffer;	//some staging memory to allow the writer to progress more in its circular queue.
										//With the current implementation, if we run out of staging buffer we loose data and proceed to the next segment.
										//Theoretically, this could be empty (no staging) in which case the reading threads can block the segments from the
										//circular buffer. This would mean that the writer is at risk of waiting in a mutex if it produces data too quickly.
										//The latter hasn't been implemented yet.

	RM_ThreadPool< TStagingThread > m_xThreadPool;
	typename TStagingThread::ThreadSharedParamGroup m_xThreadSharedParamGroup;

	//limits
	u_int m_uMaxNumRandomSegments;
	u_int m_uMaxNumPoolThreads;
};

#endif//CHALLENGE_RM_READER_PROCESS