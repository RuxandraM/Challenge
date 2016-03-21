#include "RM_OutputThread.h"
#include "RM_ThreadPool.h"

void RM_SR_OutputThread::Execute(void* pParam)
{
	printf("launched the second reader threads \n");
	if (m_xContext.IsDirty())
	{
		printf("ERR GUGU \n");
		//ResetEvent(m_xEventHandle);
		return;
	}

	const char* pSegmentMemory = m_xSharedParams.m_pSharedBuffer->GetSegmentForReading(m_xContext.GetCurrentWriteSegment());
	char* pStagingSegmentMemory = m_xSharedParams.m_pStagingBuffer->GetSegmentForWriting(m_xContext.GetCurrentStagingSegment());

	if (!pSegmentMemory || !pStagingSegmentMemory)
	{
		//ERR
		return;
	}

	memcpy(pStagingSegmentMemory, pSegmentMemory, SEGMENT_SIZE);

	char pGUGUTest[10];
	memcpy(pGUGUTest, pSegmentMemory, 9);

	//tell the main shared memory I am not reading from it anymore, for not stopping the writers that may come across this segment
	m_xSharedParams.m_pSharedBuffer->SetFinishedReading(m_xContext.GetCurrentWriteSegment());
	//tell the staging memory I am only reading from this point onwards - maybe multiple threads can write the output to hdf5?!?
	m_xSharedParams.m_pStagingBuffer->EndWritingBeginReadingToSegment(m_xContext.GetCurrentStagingSegment());


	//TODO: open and write the corresponding HDF5

	m_xSharedParams.m_pStagingBuffer->EndReadingFromSegment(m_xContext.GetCurrentStagingSegment());


	pGUGUTest[9] = '\0';
	//delay the reader too much!!

	printf("Second Reader: %s \n", pGUGUTest);

	//the thread is ready, mark it as non-active
	m_xSharedParams.m_pThreadPool->ChangeThreadToSleeping(this);
	//go back to sleep - listen for events
}