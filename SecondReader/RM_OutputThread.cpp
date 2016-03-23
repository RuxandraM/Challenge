#include "RM_OutputThread.h"
#include "../Source/RM_ThreadPool.h"


void RM_SR_OutputThread::Execute(void* pParam)
{
	if (m_xContext.IsDirty())
	{
		printf("Dirty context. A thread has been left with a dangling context. \n");
		//ResetEvent(m_xEventHandle);
		return;
	}

	const char* pSegmentMemory = m_xSharedParams.m_pSharedBuffer->GetSegmentForReading(m_xContext.GetCurrentWriteSegment());
	

	if (!pSegmentMemory)
	{
		//something went wrong, we may not be able to reserve the segment because the writer came in the way

		if (!m_xContext.GetFile() || !m_xContext.GetFileReadyEvent())
		{
			//can't write to file, I wasn't given a valid file
			printf("Failed to write to file, but also failed to retreive the shared segment from the pool \n");
		}
		else
		{
			//pass on the events or close the file
			if (m_xContext.GetFileDoneEvent()) { m_xContext.GetFileDoneEvent()->SetEvent(); }
			if (m_xContext.ShouldCloseFile())
			{
				m_xContext.GetFile()->Close();
			}
		}
	}
	else
	{
		char* pStagingSegmentMemory = m_xSharedParams.m_pStagingBuffer->GetSegmentForWriting(m_xContext.GetCurrentStagingSegment());

		if (pStagingSegmentMemory)
		{
			memcpy(pStagingSegmentMemory, pSegmentMemory, SEGMENT_SIZE);

			//tell the main shared memory I am not reading from it anymore, for not stopping the writers that may come across this segment
			m_xSharedParams.m_pSharedBuffer->SetFinishedReading(m_xContext.GetCurrentWriteSegment());

			//tell the staging memory I am only reading from this point onwards - maybe multiple threads can write the output to hdf5?!?
			m_xSharedParams.m_pStagingBuffer->EndWritingBeginReadingToSegment(m_xContext.GetCurrentStagingSegment());

			if (!m_xContext.GetFile() || !m_xContext.GetFileReadyEvent())
			{
				//can't write to file, I wasn't given a valid file
				printf("Failed to write to file \n");
			}
			else
			{
				Sleep(1500);
				m_xContext.GetFile()->WriteData(pStagingSegmentMemory, SEGMENT_SIZE);
				if (m_xContext.GetFileDoneEvent()) { m_xContext.GetFileDoneEvent()->SetEvent(); }
				if (m_xContext.ShouldCloseFile())
				{
					m_xContext.GetFile()->Close();
				}
			}
			m_xSharedParams.m_pStagingBuffer->EndReadingFromSegment(m_xContext.GetCurrentStagingSegment());
		}
	}

	//if (!pSegmentMemory || !pStagingSegmentMemory)
	//{
		//ERR
	//	return;
	//}

	//memcpy(pStagingSegmentMemory, pSegmentMemory, SEGMENT_SIZE);

	//char pGUGUTest[10];
	//memcpy(pGUGUTest, pSegmentMemory, 9);

	
	


	////TODO: open and write the corresponding HDF5
	//if (m_xContext.GetFile())
	//{
	//	if (m_xContext.GetFileReadyEvent())
	//	{
	//		m_xContext.GetFileReadyEvent()->BlockingWait();
	//		m_xContext.GetFile()->WriteData(pStagingSegmentMemory, SEGMENT_SIZE);
	//		if (m_xContext.GetFileDoneEvent()) m_xContext.GetFileDoneEvent()->SetEvent();
	//	}
	//	if (m_xContext.ShouldCloseFile())
	//	{
	//		m_xContext.GetFile()->Close();
	//	}
	//}
	//
	//m_xSharedParams.m_pStagingBuffer->EndReadingFromSegment(m_xContext.GetCurrentStagingSegment());


	//pGUGUTest[9] = '\0';
	//delay the reader too much!!

	//printf("Second Reader: %s \n", pGUGUTest);
	//----------------------------------------------------------------------------------
	//the thread is ready, mark it as non-active
	//I have to do this horrible cast because of inter-dependencies between RM_SR_OutputThread and the templated pool.
	//I should find a better way...
	//RM_ThreadPool<RM_SR_OutputThread, ThreadSharedParamGroup>* pThreadPool = 
	//	reinterpret_cast< RM_ThreadPool<RM_SR_OutputThread, ThreadSharedParamGroup>* >(m_xSharedParams.m_pThreadPool);

	RM_ThreadPool<RM_SR_OutputThread>* pThreadPool =
		reinterpret_cast< RM_ThreadPool<RM_SR_OutputThread>* >(m_xSharedParams.m_pThreadPool);

	pThreadPool->ChangeThreadToSleeping(this);
	//go back to sleep - listen for events
}


