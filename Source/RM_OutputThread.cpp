#include "RM_OutputThread.h"
#include "../Source/RM_ThreadPool.h"


void RM_SR_OutputStagingThread::ResetDerivedContext(void* pDerivedContext)
{
	Context* pxContext = reinterpret_cast<Context*>(pDerivedContext);
	m_xContext = *pxContext;
	m_xContext.ResetDirtyFlag();
}

void RM_SR_OutputStagingThread::SkipProcessing()
{
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

RM_RETURN_CODE RM_SR_OutputStagingThread::ProcessStagingMemory(char* pStagingSegmentMemory)
{
	return RM_SUCCESS;
}

RM_RETURN_CODE RM_SR_OutputStagingThread::UseStagingMemory(char* pStagingSegmentMemory)
{
	if (!m_xContext.GetFile() || !m_xContext.GetFileReadyEvent())
	{
		//can't write to file, I wasn't given a valid file
		printf("Failed to write to file ");
		if (m_xContext.GetFile()) printf(" %s \n",m_xContext.GetFile()->GetFileName());
		return RM_CUSTOM_ERR1;
	}
	
	m_xContext.GetFile()->WriteData(pStagingSegmentMemory, SEGMENT_SIZE);
	
	if (m_xContext.GetFileDoneEvent()) 
	{ 
		m_xContext.GetFileDoneEvent()->SetEvent(); 
	}

	if (m_xContext.ShouldCloseFile())
	{
		m_xContext.GetFile()->Close();
	}
	return RM_SUCCESS;
}

