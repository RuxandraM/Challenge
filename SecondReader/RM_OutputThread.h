#ifndef CHALLENGE_RM_OUTPUT_THREAD
#define CHALLENGE_RM_OUTPUT_THREAD

#include "..\Source\Utils.h"
#include "..\Source\SharedBuffer.h"
#include "..\Source\RM_SharedMemory.h"
#include "..\Source\RM_StagingBuffer.h"
#include "..\Source\RM_MessageManager.h"
#include "..\Source\RM_Thread.h"
#include "..\Source\RM_Mutex.h"

#include "SecondReader.h"

class RM_ThreadPool;

struct ThreadSharedParamGroup
{
	SharedBuffer* m_pSharedBuffer;
	RM_MessageManager<RM_CHALLENGE_PROCESS_COUNT>* m_pMessageManager;
	RM_StagingBuffer<READER_NUM_BUFFERED_SEGMENTS>* m_pStagingBuffer;
	RM_ThreadPool* m_pThreadPool;
};

struct OutputThreadContext
{
	OutputThreadContext() :
		m_iWriteTag(0),
		m_iCurrentWriteSegment(0),
		m_iStagingSegmentIndex(0),
		m_bIsDirty(true)
	{}

	OutputThreadContext(int iWriteTag, int iCurrentWriteSegment, int m_iStagingSegmentIndex) :
		m_iWriteTag(iWriteTag),
		m_iCurrentWriteSegment(iCurrentWriteSegment),
		m_iStagingSegmentIndex(m_iStagingSegmentIndex),
		m_bIsDirty(true)
	{}

	void SetValues(int iWriteTag, int iCurrentWriteSegment, int m_iStagingSegmentIndex)
	{
		m_iWriteTag = iWriteTag;
		m_iCurrentWriteSegment = iCurrentWriteSegment;
		m_iStagingSegmentIndex = m_iStagingSegmentIndex;
	}

	void ResetDirtyFlag() { m_bIsDirty = false; }
	void SetDirty() { m_bIsDirty = true; }
	bool IsDirty() { return m_bIsDirty; }


	int GetCurrentWriteSegment() const { return m_iCurrentWriteSegment; }
	int GetCurrentStagingSegment() const { return m_iStagingSegmentIndex; }


private:
	int m_iWriteTag;
	int m_iCurrentWriteSegment;
	int m_iStagingSegmentIndex;
	bool m_bIsDirty;
};

class RM_SR_OutputThread : public RM_EventThread
{
public:
	RM_SR_OutputThread(ThreadSharedParamGroup& xSharedParams) :
		m_xSharedParams(xSharedParams)
	{}

	void ResetContext(void* pContext)
	{
		OutputThreadContext* pxContext = reinterpret_cast<OutputThreadContext*>(pContext);
		m_xContext = *pxContext;
		m_xContext.ResetDirtyFlag();
	}

	void Execute(void* pParam);
	

private:
	OutputThreadContext m_xContext;
	ThreadSharedParamGroup m_xSharedParams;
	HANDLE m_xEventHandle;
};

#endif//CHALLENGE_RM_OUTPUT_THREAD