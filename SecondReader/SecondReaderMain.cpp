#include <stdio.h>
#include <windows.h>
#include "..\Source\Utils.h"
#include "..\Source\SharedBuffer.h"
#include "..\Source\RM_SharedMemory.h"
#include "..\Source\RM_StagingBuffer.h"
#include "..\Source\RM_MessageManager.h"
#include "..\Source\RM_Thread.h"
#include "..\Source\RM_Mutex.h"
#include "SecondReader.h"
#include "RM_OutputThread.h"
#include "RM_ThreadPool.h"


//static int g_iPID = 0;
static int g_iLastTag = 0;
static int g_iLastSegmentRead = 0;
static int g_iProcessIndex = 2; //TODO: this is hardcoded; it should be sent at creation




//GUGU!
//template <typename ThreadClass>








/*
class RM_SR_ListenerThread : public RM_Thread
{
public:

	RM_SR_ListenerThread(
		SharedBuffer* pSharedBuffer,
		RM_MessageManager<RM_CHALLENGE_PROCESS_COUNT>* pMessageManager,
		RM_StagingBuffer<READER_NUM_BUFFERED_SEGMENTS>* pStagingBuffer):
		m_pSharedBuffer(pSharedBuffer), m_pMessageManager(pMessageManager)
	{

	}

	int Run(void* pParam)
	{
		while (true)
		{
			int iWriteTag = m_pMessageManager->BlockingReceive(g_iProcessIndex);
			int iSegmentWritten = m_pSharedBuffer->GetLastSegmentWrittenIndex();
			int iStagingSegmentIndex = m_pStagingBuffer->ReserveSegmentIfAvailable();
			if (iStagingSegmentIndex != -1)
			{
				RM_SR_OutputThread* pOutputThread = new RM_SR_OutputThread();
				pOutputThread->Start();	//start copying to the reserved staging buffer and output to hdf5
			}
			++m_uCurrentIndex;
			if (m_uCurrentIndex == m_uNumGroupSegments)
			{
				//enqueue a reset function or so
			}
		}

		return 0;
	}

public:
	
	u_int m_uCurrentIndex;	//counts the number of consecutive reads spawned in this session
	SharedBuffer* m_pSharedBuffer;
	RM_MessageManager<RM_CHALLENGE_PROCESS_COUNT>* m_pMessageManager;
	RM_StagingBuffer<READER_NUM_BUFFERED_SEGMENTS>* m_pStagingBuffer;
};
*/



class RM_ReaderProcess
{
public:
	RM_ReaderProcess() :m_iPID(0) {}

	void Initialise()
	{
		m_iPID = _getpid();

		//GUGU
		WaitKeyPress(2);

		RM_SharedMemory xSharedMemory;
		const void* pSharedMemory = xSharedMemory.OpenMemory(RM_ACCESS_READ, SHARED_MEMORY_MAX_SIZE, TEXT(SHARED_MEMORY_NAME), m_iPID);
		RM_SharedMemory xSharedMemoryLabels;
		const void* pSharedMemoryLabels = xSharedMemoryLabels.OpenMemory(RM_ACCESS_WRITE | RM_ACCESS_READ, SHARED_MEMORY_LABESLS_MAX_SIZE,
			TEXT(SHARED_MEMORY_LABESLS_NAME), m_iPID);

		//RM_MessageManager<RM_CHALLENGE_PROCESS_COUNT> xMessageManager;
		m_xMessageManager.Initialise(m_iPID);

		m_xSharedBuffer.Initialise(const_cast<void*>(pSharedMemory), const_cast<void*>(pSharedMemoryLabels));
		printf("[%d] Reader initialised. Waiting for the writer to start \n", m_iPID);
		
	}

protected:
	int m_iPID;
	SharedBuffer m_xSharedBuffer;
	RM_MessageManager<RM_CHALLENGE_PROCESS_COUNT> m_xMessageManager;
};

class RM_SecondReader : public RM_ReaderProcess
{
public:

	RM_SecondReader() :RM_ReaderProcess(), m_uNumGroupSegments(0), m_uSegmentIndexInGroup(0) {}

	void SR_Initialise()
	{
		Initialise();	

		m_xThreadSharedParamGroup.m_pMessageManager = &m_xMessageManager;
		m_xThreadSharedParamGroup.m_pSharedBuffer = &m_xSharedBuffer;
		m_xThreadSharedParamGroup.m_pStagingBuffer = &m_xStagingBuffer;
		m_xThreadSharedParamGroup.m_pThreadPool = &m_xThreadPool;

		RM_ThreadPool xThreadPool;
		xThreadPool.Initialise(READER_NUM_POOLTHREADS, m_xThreadSharedParamGroup);

		//GUGU
		//OutputThreadContext xContext(0, 0, 0);
		xThreadPool.StartThreads();// reinterpret_cast<void*>(&xContext));
	}

	RM_RETURN_CODE ReadData()
	{
		//RM_Thread m_xListenerThread(m_xThreadSharedParamGroup);
		//m_xListenerThread.Start();
		m_uNumGroupSegments = 1 + std::rand() % READER_MAX_RANDOM_SEGMENTS;
		//keep listening
		while (true)
		{
			int iWriteTag = m_xMessageManager.BlockingReceive(g_iProcessIndex);
			int iSegmentWritten = m_xSharedBuffer.GetLastSegmentWrittenIndex();
			int iStagingSegmentIndex = m_xStagingBuffer.ReserveSegmentIfAvailable();
			if (iStagingSegmentIndex == -1)
			{
				printf("Out of staging buffer space \n");
				return RM_CUSTOM_ERR2;
			}
			//this will always return me a thread, it will create one if needed; if creation fails it means that something wrong has happened
			RM_EventThread* pOutputThread = m_xThreadPool.GetThreadForActivation(m_xThreadSharedParamGroup);
			if (!pOutputThread)
			{
				printf("No available threads \n");
				return RM_CUSTOM_ERR1;
			}
			if (iStagingSegmentIndex != -1)
			{
				//RM_SR_OutputThread* pOutputThread = new RM_SR_OutputThread(&m_xThreadSharedParamGroup, iSegmentWritten, iWriteTag, iStagingSegmentIndex);
				OutputThreadContext xContext(iWriteTag, iSegmentWritten, iStagingSegmentIndex);
				pOutputThread->ResetContext(reinterpret_cast<void*>(&xContext));
				RM_Event* pxEvent = pOutputThread->GetEvent();
				pxEvent->SetEvent();
			}
			++m_uSegmentIndexInGroup;
			if (m_uSegmentIndexInGroup == m_uNumGroupSegments)
			{
				//enqueue a reset function or so
				m_uNumGroupSegments = 1 + std::rand() % READER_MAX_RANDOM_SEGMENTS;
			}
		}
	}

	void SR_Shutdown()
	{
		m_xThreadPool.Shutdown();
	}
private:
	u_int m_uNumGroupSegments;
	u_int m_uSegmentIndexInGroup;
	RM_StagingBuffer<READER_NUM_BUFFERED_SEGMENTS> m_xStagingBuffer;
	RM_ThreadPool m_xThreadPool;
	ThreadSharedParamGroup m_xThreadSharedParamGroup;
};

//static void ReadData(SharedBuffer* pxSharedBuffer, RM_MessageManager<RM_CHALLENGE_PROCESS_COUNT>* pxMessageManager)
//{
//	u_int uNumElememts = std::rand() % READER_MAX_RANDOM_SEGMENTS;
//
//	RM_Thread m_xListenerThread;
//	m_xListenerThread.Start();
//
//	RM_ThreadPool xThreadPool;
//	xThreadPool.Initialise(READER_NUM_POOLTHREADS);
//
//	
//
//	int iWriteTag = xMessageManager.BlockingReceive(g_iProcessIndex);
//	int iSegmentWritten = xSharedBuffer.GetLastSegmentWrittenIndex();
//	printf("[%d] Received signal for segment %d, tag %d, attempting to read %d segment from now on \n", g_iPID, iSegmentWritten, iWriteTag, uNumElememts);
//}



int main()
{
	//g_iPID = _getpid();

	RM_SecondReader xReader;
	xReader.SR_Initialise();

	xReader.ReadData();

	xReader.SR_Shutdown();
	/*
	RM_SharedMemory xSharedMemory;
	const void* pSharedMemory = xSharedMemory.OpenMemory(RM_ACCESS_READ, SHARED_MEMORY_MAX_SIZE, TEXT(SHARED_MEMORY_NAME), g_iPID);
	RM_SharedMemory xSharedMemoryLabels;
	const void* pSharedMemoryLabels = xSharedMemoryLabels.OpenMemory(RM_ACCESS_WRITE | RM_ACCESS_READ, SHARED_MEMORY_LABESLS_MAX_SIZE,
		TEXT(SHARED_MEMORY_LABESLS_NAME), g_iPID);

	RM_MessageManager<RM_CHALLENGE_PROCESS_COUNT> xMessageManager;
	xMessageManager.Initialise(g_iPID);

	SharedBuffer xSharedBuffer(const_cast<void*>(pSharedMemory), const_cast<void*>(pSharedMemoryLabels));
	printf("[%d] Second Reader initialised. Waiting for the writer to start \n", g_iPID);



	ReadData(xSharedBuffer, xMessageManager);
	*/
	printf("Second Reader done. Waiting key press... \n");
	WaitKeyPress(2);

	return S_OK;
}