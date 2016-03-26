#include "RM_ReaderOutputThread.h"
#include "RM_ThreadPool.h"

void RM_OutputStagingThread::Execute(void* pParam)
{
	if (AreContextsDirty()) 
	{
		printf("Dirty context. A thread has been left with a dangling context. \n");
		return; 
	}

	const char* pSegmentMemory = m_xSharedParams.m_pSharedBuffer->GetSegmentForReading(m_xStagingContext.GetCurrentWriteSegment());


	if (!pSegmentMemory)
	{
		//something went wrong, we may not be able to reserve the segment because the writer came in the way
		SkipProcessing();
	}
	else
	{
		char* pStagingSegmentMemory = m_xSharedParams.m_pStagingBuffer->GetSegmentForWriting(m_xStagingContext.GetCurrentStagingSegment());

		if (pStagingSegmentMemory)
		{
			memcpy(pStagingSegmentMemory, pSegmentMemory, SEGMENT_SIZE);

			//tell the main shared memory I am not reading from it anymore, for not stopping the writers that may come across this segment
			m_xSharedParams.m_pSharedBuffer->SetFinishedReading(m_xStagingContext.GetCurrentWriteSegment());

			RM_RETURN_CODE xResult = ProcessStagingMemory(pStagingSegmentMemory);

			//tell the staging memory I am only reading from this point onwards - maybe multiple threads can write the output to hdf5?!?
			m_xSharedParams.m_pStagingBuffer->EndWritingBeginReadingToSegment(m_xStagingContext.GetCurrentStagingSegment());

			if (xResult == RM_SUCCESS)
			{
				UseStagingMemory(pStagingSegmentMemory);
			}
			
			m_xSharedParams.m_pStagingBuffer->EndReadingFromSegment(m_xStagingContext.GetCurrentStagingSegment());
		}
	}

	//----------------------------------------------------------------------------------
	//the thread is ready, mark it as non-active
	//I have to do this horrible cast because of inter-dependencies between RM_OutputStagingThread and the templated pool.
	//I should find a better way...
	RM_ThreadPool<RM_OutputStagingThread>* pThreadPool =
		reinterpret_cast< RM_ThreadPool<RM_OutputStagingThread >* >(m_xSharedParams.m_pThreadPool);
	pThreadPool->ChangeThreadToSleeping(this);
	//go back to sleep - listen for events
}


