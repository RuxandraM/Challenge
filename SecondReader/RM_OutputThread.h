#ifndef CHALLENGE_RM_OUTPUT_THREAD
#define CHALLENGE_RM_OUTPUT_THREAD

#include "..\Source\Utils.h"
#include "..\Source\RM_SharedBuffer.h"
#include "..\Source\RM_File.h"
#include "..\Source\RM_SharedMemory.h"
#include "..\Source\RM_StagingBuffer.h"
#include "..\Source\RM_MessageManager.h"
#include "..\Source\RM_Thread.h"
#include "..\Source\RM_Mutex.h"
#include "..\Source\RM_ReaderOutputThread.h"
#include "SecondReader.h"


class RM_File;

class RM_SR_OutputStagingThread : public RM_OutputStagingThread
{
public:
	
	//-----------------------------------------------------------------
	class Context
	{
	public:
		Context() :
			m_pxFile(nullptr),
			m_pxFileReadyEvent(nullptr),
			m_pxFileDoneEvent(nullptr),
			m_bShouldCloseFile(false),
			m_bIsDirty(true)
		{}

		Context(RM_File* pxFile, RM_Event* pxFileReadyEvent, RM_Event* pxFileDoneEvent, bool bShouldCloseFile) :
			m_pxFile(pxFile),
			m_pxFileReadyEvent(pxFileReadyEvent),
			m_pxFileDoneEvent(pxFileDoneEvent),
			m_bShouldCloseFile(bShouldCloseFile),
			m_bIsDirty(true)
		{}

		void ResetDirtyFlag() { m_bIsDirty = false; }
		void SetDirty() { m_bIsDirty = true; }
		bool IsDirty() { return m_bIsDirty; }

		RM_File* GetFile() const { return m_pxFile; }
		RM_Event* GetFileReadyEvent() const { return m_pxFileReadyEvent; }
		RM_Event* GetFileDoneEvent() const { return m_pxFileDoneEvent; }
		bool ShouldCloseFile() const { return m_bShouldCloseFile; }

	private:
		RM_File* m_pxFile;
		RM_Event* m_pxFileReadyEvent;
		RM_Event* m_pxFileDoneEvent;
		bool m_bShouldCloseFile;
		bool m_bIsDirty;
	};
	//-----------------------------------------------------------------------------------------
	RM_SR_OutputStagingThread(ThreadSharedParamGroup* pSharedParams) :RM_OutputStagingThread(pSharedParams) {}

	virtual void SkipProcessing();

	virtual RM_RETURN_CODE ProcessStagingMemory(char* pStagingSegmentMemory);

	virtual RM_RETURN_CODE UseStagingMemory(char* pStagingSegmentMemory);

	void ResetContext(void* pContext)
	{
		Context* pxContext = reinterpret_cast<Context*>(pContext);
		m_xContext = *pxContext;
		m_xContext.ResetDirtyFlag();
	}

protected:
	virtual bool IsDerivedContextDirty() { return m_xContext.IsDirty(); }
	virtual void ResetDerivedContext(void* pDerivedContext);

protected:
	Context m_xContext;
};


class RM_SR_WorkerThread : public RM_WorkerListenerThread
{
public:
	struct ThreadSharedParamGroup
	{
		RM_File* m_pxFile;
	};

	explicit RM_SR_WorkerThread(ThreadSharedParamGroup* pxSharedParams): m_pxSharedParams(pxSharedParams){}

	void Execute(void* pParam) 
	{ 
		if (m_pxSharedParams && m_pxSharedParams->m_pxFile) m_pxSharedParams->m_pxFile->Open(true);
		InternalStartShutdown();
	}
private:
	ThreadSharedParamGroup* m_pxSharedParams;
};

#endif//CHALLENGE_RM_OUTPUT_THREAD