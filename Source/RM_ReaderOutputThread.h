#ifndef CHALLENGE_RM_READER_OUTPUT_THREAD
#define CHALLENGE_RM_READER_OUTPUT_THREAD

#include "..\Source\Utils.h"
#include "..\Source\SharedBuffer.h"
#include "..\Source\RM_File.h"
#include "..\Source\RM_SharedMemory.h"
#include "..\Source\RM_StagingBuffer.h"
#include "..\Source\RM_MessageManager.h"
#include "..\Source\RM_Thread.h"
#include "..\Source\RM_Mutex.h"


//class RM_ThreadPool;
class RM_File;






class RM_OutputStagingThread : public RM_WorkerListenerThread
{
public:

	struct ThreadSharedParamGroup
	{
		SharedBuffer* m_pSharedBuffer;
		RM_MessageManager<RM_CHALLENGE_PROCESS_COUNT, RM_WToRMessageData>* m_pMessageManager;
		RM_StagingBuffer* m_pStagingBuffer;
		void* m_pThreadPool;	//RM_ThreadPool
	};

	class Context
	{
	public:
		Context() :
			m_iWriteTag(0),
			m_iCurrentWriteSegment(0),
			m_iStagingSegmentIndex(0),
			//m_pxFile(nullptr),
			//m_pxFileReadyEvent(nullptr),
			m_pxFileDoneEvent(nullptr),
			//m_bShouldCloseFile(false),
			m_bIsDirty(true)
		{}

		Context(int iWriteTag, int iCurrentWriteSegment, int m_iStagingSegmentIndex,
			RM_File* pxFile, RM_Event* pxFileReadyEvent, RM_Event* pxFileDoneEvent, bool bShouldCloseFile) :
			m_iWriteTag(iWriteTag),
			m_iCurrentWriteSegment(iCurrentWriteSegment),
			m_iStagingSegmentIndex(m_iStagingSegmentIndex),
			//m_pxFile(pxFile),
			//m_pxFileReadyEvent(pxFileReadyEvent),
			m_pxFileDoneEvent(pxFileDoneEvent),
			//m_bShouldCloseFile(bShouldCloseFile),
			m_bIsDirty(true)
		{}

		//void SetValues(int iWriteTag, int iCurrentWriteSegment, int m_iStagingSegmentIndex)
		//{
		//	m_iWriteTag = iWriteTag;
		//	m_iCurrentWriteSegment = iCurrentWriteSegment;
		//	m_iStagingSegmentIndex = m_iStagingSegmentIndex;
		//}

		void ResetDirtyFlag() { m_bIsDirty = false; }
		void SetDirty() { m_bIsDirty = true; }
		bool IsDirty() { return m_bIsDirty; }


		int GetCurrentWriteSegment() const { return m_iCurrentWriteSegment; }
		int GetCurrentStagingSegment() const { return m_iStagingSegmentIndex; }

		RM_File* GetFile() const { return m_pxFile; }
		RM_Event* GetFileReadyEvent() const { return m_pxFileReadyEvent; }
		RM_Event* GetFileDoneEvent() const { return m_pxFileDoneEvent; }
		bool ShouldCloseFile() const { return m_bShouldCloseFile; }

	private:
		int m_iWriteTag;
		int m_iCurrentWriteSegment;
		int m_iStagingSegmentIndex;
		RM_File* m_pxFile;
		RM_Event* m_pxFileReadyEvent;
		RM_Event* m_pxFileDoneEvent;
		bool m_bShouldCloseFile;
		bool m_bIsDirty;
	};

	RM_OutputStagingThread(ThreadSharedParamGroup* pxSharedParams) :
		m_xSharedParams(*pxSharedParams)
	{}

	void ResetContext(void* pContext)
	{
		Context* pxContext = reinterpret_cast<Context*>(pContext);
		m_xContext = *pxContext;
		m_xContext.ResetDirtyFlag();
	}

	void Execute(void* pParam);


private:
	Context m_xContext;
	ThreadSharedParamGroup m_xSharedParams;
	HANDLE m_xEventHandle;
};


#endif//CHALLENGE_RM_READER_OUTPUT_THREAD