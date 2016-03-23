#ifndef CHALLENGE_RM_READER_PROCESS
#define CHALLENGE_RM_READER_PROCESS

#include <stdio.h>
#include <windows.h>
#include "..\Source\Utils.h"
#include "..\Source\SharedBuffer.h"
#include "..\Source\RM_SharedMemory.h"
#include "..\Source\RM_StagingBuffer.h"
#include "..\Source\RM_MessageManager.h"
#include "..\Source\RM_Thread.h"
#include "RM_ReaderOutputThread.h"
#include "RM_ThreadPool.h"

class RM_ReaderProcess
{
public:
	RM_ReaderProcess() :m_iPID(0) {}

	void Initialise()
	{
		m_iPID = _getpid();

		RM_SharedMemory xSharedMemory;
		void* pSharedMemory = xSharedMemory.OpenMemory(RM_ACCESS_READ, SHARED_MEMORY_MAX_SIZE, TEXT(SHARED_MEMORY_NAME), m_iPID);
		RM_SharedMemory xSharedMemoryLabels;
		void* pSharedMemoryLabels = xSharedMemoryLabels.OpenMemory(RM_ACCESS_WRITE | RM_ACCESS_READ, SHARED_MEMORY_LABESLS_MAX_SIZE,
			TEXT(SHARED_MEMORY_LABESLS_NAME), m_iPID);

		m_xMessageManager.Initialise(m_iPID);

		m_xSharedBuffer.MapMemory(pSharedMemory, pSharedMemoryLabels);
		printf("[%d] Reader initialised. Waiting for the writer to start \n", m_iPID);
	}

protected:
	int m_iPID;
	SharedBuffer m_xSharedBuffer;
	RM_MessageManager<RM_CHALLENGE_PROCESS_COUNT, RM_WToRMessageData> m_xMessageManager;
};

template<u_int uNUM_STAGING_SEGMENTS>
class RM_StagingReader : public RM_ReaderProcess
{
public:

	RM_StagingReader(int iProcessIndex, u_int uMaxNumRandomSegments, u_int uMaxNumPoolThreads) :RM_ReaderProcess(),
		m_uNumGroupSegments(0),
		m_uSegmentIndexInGroup(0),
		//m_uCurrentFile(0),
		m_uCurrentGroupIteration(0),
		//m_uSegmentIndexInFile(0)
		m_iProcessIndex(iProcessIndex),
		m_uMaxNumRandomSegments(uMaxNumRandomSegments),
		m_uMaxNumPoolThreads(uMaxNumPoolThreads)
	{
		//memset(&m_xFileLimits, 0, sizeof(FileLimits));
	}

	void SR_Initialise()
	{
		Initialise();

		m_xThreadSharedParamGroup.m_pMessageManager = &m_xMessageManager;
		m_xThreadSharedParamGroup.m_pSharedBuffer = &m_xSharedBuffer;
		m_xThreadSharedParamGroup.m_pStagingBuffer = &m_xStagingBuffer;
		m_xThreadSharedParamGroup.m_pThreadPool = &m_xThreadPool;

		m_xThreadPool.Initialise(m_uMaxNumPoolThreads, &m_xThreadSharedParamGroup);
		m_xStagingBuffer.Initialise();

		m_xThreadPool.StartThreads();

		//m_xFileOpenWorkerPool.Initialise(READER_MAX_NUM_FILES, nullptr);
		//m_xFileOpenWorkerPool.StartThreads();
	}

	void StartNewGroup()
	{
		m_uNumGroupSegments = 1 + std::rand() % m_uMaxNumRandomSegments;
		/*
		m_xFileLimits = GetFileLimits(m_uNumGroupSegments);
		//if we have more data than we can ever output to files, clamp the total number of segments
		m_uNumGroupSegments = m_xFileLimits.m_uNumTotalSegments;

		//start m_xFileLimits.m_uNumFiles threads that create and open the files
		//each will signal an event when they are ready
		if (m_xGroupFiles.size())
		{
			//error
			for (std::vector<RM_File*>::iterator it = m_xGroupFiles.begin(); it != m_xGroupFiles.end(); ++it)
			{
				RM_File *pFile = *it;
				//TODO
				//if (pFile->IsOpen()) pFile->Close();
				delete pFile;
			}
		}
		m_xGroupFiles.clear();

		//start n worker threads that will create and open the files
		for (u_int u = 0; u < m_xFileLimits.m_uNumFiles; ++u)
		{
			std::string xFileName(READER_OUTPUT_NAME);
			char szBuffer[32];
			_itoa_s(m_uCurrentGroupIteration, szBuffer, 10);
			xFileName.append(szBuffer);
			RM_File* pFile = new RM_File(xFileName.c_str(), (u_int)xFileName.length());
			m_xGroupFiles.push_back(pFile);
			RM_SR_WorkerThread* pThread = m_xFileOpenWorkerPool.GetThreadForActivation(pFile);
			RM_Event* pStartEvent = pThread->GetEvent();
			pFile->UpdateIsReadyEvent(pThread->GetFinishEvent());
			pStartEvent->SetEvent();	//launch the new thread that will open the file
		}
		*/
	}

	RM_RETURN_CODE ReadData()
	{
		//start a new iteration; this will generate the number of segments in the group, and will reset the counters
		StartNewGroup();
		/*
		size_t xSize = m_xGroupFiles.size();
		if (xSize == 0)
		{
			printf("Failed to open any files. The reader will not execute. \n");
			return RM_CUSTOM_ERR1;
		}
		RM_File* pCurrentFile = m_xGroupFiles[xSize - 1];
		m_xGroupFiles.pop_back();
		RM_Event* pCurrentIsFileReadyEvent = pCurrentFile->GetIsReadyEvent();
		RM_Event* pCurrentIsFileUpdatedEvent = nullptr;
		*/
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

			//printf("Reading segment num %d, index %d \n",xMessage.m_uTag,xMessage.m_uSegmentWritten);

			//int iWriteTag = m_xMessageManager.BlockingReceive(g_iProcessIndex);
			//int iSegmentWritten = m_xSharedBuffer.GetLastSegmentWrittenIndex();
			int iStagingSegmentIndex = m_xStagingBuffer.ReserveSegmentIfAvailable();
			if (iStagingSegmentIndex == -1)
			{
				printf("[%d] Out of staging buffer space \n", m_iPID);
				return RM_CUSTOM_ERR2;
			}
			//this will always return me a thread, it will create one if needed; if creation fails it means that something wrong has happened
			RM_WorkerListenerThread* pOutputThread = m_xThreadPool.GetThreadForActivation(&m_xThreadSharedParamGroup);
			if (!pOutputThread)
			{
				printf("No available threads \n");
				return RM_CUSTOM_ERR1;
			}
			if (iStagingSegmentIndex != -1)
			{
				//Every thread needs to know the current file, the event to listen when my output turn comes, and the 
				//event I need to send when I'm done with outputting
				//pCurrentIsFileUpdatedEvent = pOutputThread->GetFinishEvent();
				//const bool bShouldCloseFile = ((m_uSegmentIndexInFile + 1) == m_xFileLimits.m_uNumSegmentsPerFile);

				OutputThreadContext xContext(xMessage.m_uTag, xMessage.m_uSegmentWritten, iStagingSegmentIndex); 
					//pCurrentFile, pCurrentIsFileReadyEvent, pCurrentIsFileUpdatedEvent, bShouldCloseFile);
				pOutputThread->ResetContext(reinterpret_cast<void*>(&xContext));
				RM_Event* pxEvent = pOutputThread->GetEvent();
				pxEvent->SetEvent();

				//pCurrentIsFileReadyEvent = pCurrentIsFileUpdatedEvent;
				//pCurrentIsFileUpdatedEvent = nullptr;
			}
			else
			{
				//TODO:
				//I could try reading straight from the shared buffer, hoping that the circuar character of the buffer will allow me
				//to read without interfering with the writer.
				printf("[%d] Out of staging buffer space. I will skip reading this segment.\n", m_iPID);
				return RM_CUSTOM_ERR1;
			}

			//count the consecutive segments since the start of this group
			++m_uSegmentIndexInGroup;
			//count the segments since the start of the file
			//++m_uSegmentIndexInFile;

			//if (m_uSegmentIndexInFile == m_xFileLimits.m_uNumSegmentsPerFile)
			//{
			//	m_uSegmentIndexInFile = 0;
			//	//get new file
			//	size_t xSize = m_xGroupFiles.size();
			//	if (xSize == 0)
			//	{
			//		//no more files to process. Reached the end of the group.
			//		pCurrentFile = nullptr;
			//	}
			//	else
			//	{
			//		pCurrentFile = m_xGroupFiles[xSize - 1];
			//		m_xGroupFiles.pop_back();
			//		pCurrentIsFileReadyEvent = pCurrentFile->GetIsReadyEvent();
			//	}
			//}

			//----------------------------------------------------------------------
			//TODO: a thread has the file and it will need to close it. I will not wait here to close, just delete the pointers
			//TODO: store a list of files that need closing. The threads will pop from here when they close the file
			if (m_uSegmentIndexInGroup == m_uNumGroupSegments)
			{
				//for (std::vector<RM_File*>::iterator it = m_xGroupFiles.begin(); it != m_xGroupFiles.end(); ++it)
				//{
				//	RM_File *pFile = *it;
				//	//TODO
				//	//if (pFile->IsOpen()) pFile->Close();
				//	delete pFile;
				//}
				//m_xGroupFiles.clear();

				++m_uCurrentGroupIteration;

				StartNewGroup();

				//size_t xSize = m_xGroupFiles.size();
				//if (xSize == 0)
				//{
				//	printf("Failed to open any files. The reader will not execute. \n");
				//	return RM_CUSTOM_ERR1;
				//}
				//pCurrentFile = m_xGroupFiles[xSize - 1];
				//m_xGroupFiles.pop_back();
				//pCurrentIsFileReadyEvent = pCurrentFile->GetIsReadyEvent();
				//pCurrentIsFileUpdatedEvent = nullptr;
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
	u_int m_uCurrentFile;
	u_int m_uCurrentGroupIteration;
	u_int m_iProcessIndex;

	RM_StagingBuffer<uNUM_STAGING_SEGMENTS> m_xStagingBuffer;
	RM_ThreadPool<RM_OutputStagingThread<uNUM_STAGING_SEGMENTS>, StagingThreadSharedParamGroup<uNUM_STAGING_SEGMENTS> > m_xThreadPool;
	StagingThreadSharedParamGroup<uNUM_STAGING_SEGMENTS> m_xThreadSharedParamGroup;

	//limits
	u_int m_uMaxNumRandomSegments;
	u_int m_uMaxNumPoolThreads;

	//RM_ThreadPool<RM_SR_WorkerThread, RM_File> m_xFileOpenWorkerPool;
	//std::vector<RM_File*> m_xGroupFiles;//GUGU: mem leaks??
	//u_int m_uSegmentIndexInFile;
	//FileLimits m_xFileLimits;
};

#endif//CHALLENGE_RM_READER_PROCESS