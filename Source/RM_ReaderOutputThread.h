#ifndef CHALLENGE_RM_READER_OUTPUT_THREAD
#define CHALLENGE_RM_READER_OUTPUT_THREAD

#include "../Source/Utils.h"
#include "../Source/RM_SharedBuffer.h"
#include "../Source/RM_SharedMemory.h"
#include "../Source/RM_StagingBuffer.h"
#include "../Source/RM_MessageManager.h"
#include "../Source/RM_Thread.h"
#include "../Source/RM_Mutex.h"


//Abstract class that encapsulates the logic for copying to a staging segment, then calling
//pure virtuals to process the resulting staging buffer. It needs to be a RM_WorkerListenerThread
//because it will signal when it is done (i.e. have processed the staging memory)
//It has to have ThreadSharedParamGroup and StagingContext because of the way the reader initialises it.
class RM_OutputStagingThread : public RM_WorkerListenerThread
{
public:
	//--------------------------------------------------------------------------
	//nested structs/classes
	//--------------------------------------------------------------------------
	struct ThreadSharedParamGroup
	{
		RM_SharedBuffer* m_pSharedBuffer;
		RM_StagingBuffer* m_pStagingBuffer;
		void* m_pThreadPool;	//RM_ThreadPool
	};

	class StagingContext
	{
	public:
		StagingContext():
			m_uWriteTag(0u),
			m_iCurrentWriteSegment(0),
			m_iStagingSegmentIndex(0),
			m_bIsDirty(true)
		{}

		StagingContext(u_int uWriteTag, int iCurrentWriteSegment, int m_iStagingSegmentIndex) :
			m_uWriteTag(uWriteTag),
			m_iCurrentWriteSegment(iCurrentWriteSegment),
			m_iStagingSegmentIndex(m_iStagingSegmentIndex),
			m_bIsDirty(true)
		{}

		void ResetDirtyFlag() { m_bIsDirty = false; }
		void SetDirty() { m_bIsDirty = true; }
		bool IsDirty() { return m_bIsDirty; }

		u_int GetWriteTag() const { return m_uWriteTag; }
		int GetCurrentWriteSegment() const { return m_iCurrentWriteSegment; }
		int GetCurrentStagingSegment() const { return m_iStagingSegmentIndex; }

	private:
		u_int m_uWriteTag;
		int m_iCurrentWriteSegment;
		int m_iStagingSegmentIndex;
		bool m_bIsDirty;
	};

	//--------------------------------------------------------------------------
	//end of nested objects
	//--------------------------------------------------------------------------

	RM_OutputStagingThread(ThreadSharedParamGroup* pxSharedParams) :
		m_xSharedParams(*pxSharedParams)
	{}

	bool AreContextsDirty()
	{
		return (m_xStagingContext.IsDirty() || IsDerivedContextDirty());
	}
	void ResetContexts(StagingContext& xStagingContext, void* pDerivedContext)
	{
		m_xStagingContext = xStagingContext;
		m_xStagingContext.ResetDirtyFlag();
		ResetDerivedContext(pDerivedContext);
	}
	

	virtual void Execute(void* pParam);
	
	virtual void SkipProcessing() = 0;
	virtual RM_RETURN_CODE ProcessStagingMemory(char* pStagingSegmentMemory) = 0;
	virtual RM_RETURN_CODE UseStagingMemory(char* pStagingSegmentMemory) = 0;

protected:
	virtual bool IsDerivedContextDirty() = 0;
	virtual void ResetDerivedContext(void* pDerivedContext) = 0;

protected:
	StagingContext m_xStagingContext;
	ThreadSharedParamGroup m_xSharedParams;
};


#endif//CHALLENGE_RM_READER_OUTPUT_THREAD