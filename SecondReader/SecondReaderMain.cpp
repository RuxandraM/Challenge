#include <stdio.h>
#include <windows.h>
#include "../Source/Utils.h"
#include "../Source/RM_SharedBuffer.h"
#include "../Source/RM_ReaderProcess.h"
#include "../Source/RM_SharedMemory.h"
#include "../Source/RM_StagingBuffer.h"
#include "../Source/RM_MessageManager.h"
#include "../Source/RM_Thread.h"
#include "../Source/RM_Mutex.h"
#include "../Source/RM_ThreadPool.h"
#include "../Source/RM_ReaderProcess.h"
#include "../Source/RM_OutputThread.h"
#include "SecondReader.h"
#include <vector>


static int g_iProcessIndex = 2; //TODO: this is hardcoded; it should be sent at creation


template<typename TStagingThread>
class RM_SecondReader : public RM_StagingReader<TStagingThread>
{
public:
	typedef RM_StagingReader<TStagingThread> PARENT;

	RM_SecondReader(int iProcessIndex, u_int uMaxNumRandomSegments, u_int uMaxNumPoolThreads, u_int uNumStagingSegments) :
		RM_StagingReader(iProcessIndex, uMaxNumRandomSegments, uMaxNumPoolThreads, uNumStagingSegments),
		m_pCurrentFile(nullptr),
		m_pCurrentIsFileReadyEvent(nullptr),
		m_pCurrentIsFileUpdatedEvent(nullptr),
		m_uSegmentIndexInFile(0)
	{
		memset(&m_xFileLimits, 0, sizeof(FileLimits));
	}

	void Initialise()
	{
		PARENT::Initialise();

		m_xFileOpenWorkerPool.Initialise(READER_MAX_NUM_FILES, nullptr);
		m_xFileOpenWorkerPool.StartThreads();
	}

	virtual void ResetThreadContext(TStagingThread* pStagingThread, typename TStagingThread::StagingContext& xStagingContext)
	{
		m_pCurrentIsFileUpdatedEvent = pStagingThread->GetFinishEvent();
		const bool bShouldCloseFile = ((m_uSegmentIndexInFile + 1) == m_xFileLimits.m_uNumSegmentsPerFile) || 
			((m_uSegmentIndexInGroup + 1) == m_xFileLimits.m_uNumTotalSegments);	//if it reached the end of the current file or the end of the group
		TStagingThread::Context xDerivedContext(m_pCurrentFile, m_pCurrentIsFileReadyEvent, m_pCurrentIsFileUpdatedEvent, bShouldCloseFile);
		//TODO: I should be able to send the fully typed context here
		pStagingThread->ResetContexts(xStagingContext, &xDerivedContext);
	}

	virtual void CloseThreadContext()
	{
		m_pCurrentIsFileReadyEvent = m_pCurrentIsFileUpdatedEvent;
		m_pCurrentIsFileUpdatedEvent = nullptr;
	}
	virtual void EndIteration()
	{
		//count the segments since the start of the file
		++m_uSegmentIndexInFile;
		if (m_uSegmentIndexInGroup == m_xFileLimits.m_uNumTotalSegments)
		{
			//gropup ended
			m_uSegmentIndexInFile = 0;
			m_pCurrentFile = nullptr;
		}

		//if we reached the end of the file due to num segments per file limit
		if (m_uSegmentIndexInFile == m_xFileLimits.m_uNumSegmentsPerFile )
		{
			m_uSegmentIndexInFile = 0;
			//get new file
			size_t xSize = m_xGroupFiles.size();
			if (xSize == 0)
			{
				//no more files to process. Reached the end of the group.
				m_pCurrentFile = nullptr;
			}
			else
			{
				m_pCurrentFile = m_xGroupFiles[xSize - 1];
				m_xGroupFiles.pop_back();
				m_pCurrentIsFileReadyEvent = m_pCurrentFile->GetIsReadyEvent();
			}
		}
	}

	virtual RM_RETURN_CODE CustomReaderStartNewGroup()
	{
		m_xFileLimits = GetFileLimits( m_uNumGroupSegments );
		//if we have more data than we can ever output to files, clamp the total number of segments
		m_uNumGroupSegments = m_xFileLimits.m_uNumTotalSegments;

		printf("Starting group number %d with %d segments to be written in %d files\n",
			m_uCurrentGroupIteration, m_uNumGroupSegments,m_xFileLimits.m_uNumFiles);

		//start m_xFileLimits.m_uNumFiles threads that create and open the files
		//each will signal an event when they are ready
		if (m_xGroupFiles.size())
		{
			//error
			for (std::vector<RM_File*>::iterator it = m_xGroupFiles.begin(); it != m_xGroupFiles.end(); ++it)
			{
				RM_File *pFile = *it;
				//don't close the file here. A thread will do so when it's done with it. TODO: We should however be more secure an 
				//don't risk leaving dangling files that are pending closing.
				//if (pFile->IsOpen()) pFile->Close();
				delete pFile;
			}
		}
		m_xGroupFiles.clear();

		//start n worker threads that will create and open the files
		for (u_int uFile = 0; uFile < m_xFileLimits.m_uNumFiles; ++uFile)
		{
			std::string xFileName(m_xGlobalOutputFileName);
			char szBuffer[32];
			_itoa_s(m_uCurrentGroupIteration, szBuffer, 10);
			xFileName.append(szBuffer);
			xFileName.append("_");
			_itoa_s(m_xFileLimits.m_uNumFiles - uFile - 1, szBuffer, 10);
			xFileName.append(szBuffer);

			RM_File* pFile = new RM_File(xFileName.c_str(), (u_int)xFileName.length(),RM_ACCESS_WRITE);
			m_xGroupFiles.push_back(pFile);
			
			RM_SR_WorkerThread* pThread = m_xFileOpenWorkerPool.GetThreadForActivation(nullptr);
			RM_SR_WorkerThread::Context xContext(pFile, m_uCurrentGroupIteration);
			pThread->ResetContext(&xContext);
			RM_Event* pStartEvent = pThread->GetEvent();
			pFile->UpdateIsReadyEvent(pThread->GetFinishEvent());
			pStartEvent->SetEvent();	//launch the new thread that will open the file
		}

		size_t xSize = m_xGroupFiles.size();
		if (xSize == 0)
		{
			printf("Failed to open any files. The reader will not execute. \n");
			return RM_CUSTOM_ERR1;
		}
		m_pCurrentFile = m_xGroupFiles[xSize - 1];
		m_xGroupFiles.pop_back();
		m_pCurrentIsFileReadyEvent = m_pCurrentFile->GetIsReadyEvent();
		m_pCurrentIsFileUpdatedEvent = nullptr;
		return RM_SUCCESS;
	}

	void Shutdown()
	{
		PARENT::Shutdown();
		m_xFileOpenWorkerPool.Shutdown();

		if (m_xGroupFiles.size())
		{
			//error
			for (std::vector<RM_File*>::iterator it = m_xGroupFiles.begin(); it != m_xGroupFiles.end(); ++it)
			{
				RM_File *pFile = *it;
				if (pFile->IsOpen()) pFile->Close();
				delete pFile;
			}
		}
		m_xGroupFiles.clear();
	}

private:
	RM_File* m_pCurrentFile;
	RM_Event* m_pCurrentIsFileReadyEvent;
	RM_Event* m_pCurrentIsFileUpdatedEvent;


	RM_ThreadPool<RM_SR_WorkerThread> m_xFileOpenWorkerPool;
	std::vector<RM_File*> m_xGroupFiles;
	u_int m_uSegmentIndexInFile;
	FileLimits m_xFileLimits;
};

int main()
{
	RM_SecondReader<RM_SR_OutputStagingThread> xReader(
		g_iProcessIndex,
		READER_MAX_RANDOM_SEGMENTS,
		READER_NUM_POOLTHREADS,
		READER_NUM_BUFFERED_SEGMENTS);


	xReader.Initialise();

	xReader.ReadData();

	xReader.Shutdown();
	
	printf("Second Reader done. Waiting key press... \n");
	WaitKeyPress(2);

	return S_OK;
}