#include <stdio.h>
#include <windows.h>
#include "..\Source\Utils.h"
#include "..\Source\RM_SharedBuffer.h"
#include "..\Source\RM_SharedMemory.h"
#include "..\Source\RM_StagingBuffer.h"
#include "..\Source\RM_MessageManager.h"
#include "..\Source\RM_Thread.h"
#include "..\Source\RM_Mutex.h"
#include "..\Source\RM_ReaderOutputThread.h"
#include "..\Source\RM_ThreadPool.h"
#include "..\Source\RM_ReaderProcess.h"
#include "ThirdReader.h"
#include <vector>


static int g_iProcessIndex = 3; //TODO: this is hardcoded; it should be sent at creation

class RM_TR_ProcessingStagingThread : public RM_OutputStagingThread
{
public:

	//-----------------------------------------------------------------
	class Context
	{
	public:
		Context() :
			m_pxDuplicateBuffer(nullptr),
			m_pxMessageManagerDuplicate(nullptr),
			m_iProcessIndex(0),
			m_bIsDirty(true)
		{}

		Context( RM_SharedBuffer* pxDuplicateBuffer, 
			RM_MessageManager<RM_CHALLENGE_PROCESS_COUNT, RM_WToRMessageData>* pxMessageManagerDuplicate, int iProcessIndex) :
			m_pxDuplicateBuffer(pxDuplicateBuffer),
			m_pxMessageManagerDuplicate(pxMessageManagerDuplicate),
			m_iProcessIndex(iProcessIndex),
			m_bIsDirty(true)
		{}

		void ResetDirtyFlag() { m_bIsDirty = false; }
		void SetDirty() { m_bIsDirty = true; }
		bool IsDirty() const { return m_bIsDirty; }
	
		
		RM_SharedBuffer* GetDuplicateBuffer() const { return m_pxDuplicateBuffer; }
		RM_MessageManager<RM_CHALLENGE_PROCESS_COUNT, RM_WToRMessageData>* GetMessageManager() const { return m_pxMessageManagerDuplicate; }
		int GetProcessIndex() const { return m_iProcessIndex; }
		//u_int GetCurrentTag() const { return m_uWriteTag; }

	private:
		
		//u_int m_uWriteTag;
		RM_SharedBuffer* m_pxDuplicateBuffer;
		RM_MessageManager<RM_CHALLENGE_PROCESS_COUNT, RM_WToRMessageData>* m_pxMessageManagerDuplicate;
		u_int m_iProcessIndex;	//the index of the process who is sending the messages after the data was processed
		bool m_bIsDirty;
	};
	//-----------------------------------------------------------------------------------------
	RM_TR_ProcessingStagingThread(ThreadSharedParamGroup* pSharedParams) :RM_OutputStagingThread(pSharedParams) {}

	virtual void SkipProcessing() {};
	
	virtual char ProcessElement(u_int u, char* pStagingSegmentMemory)
	{
		//any operation here
		return pStagingSegmentMemory[u] + pStagingSegmentMemory[(u + 5) % SEGMENT_SIZE];
	}

	virtual RM_RETURN_CODE ProcessStagingMemory(char* pStagingSegmentMemory)
	{
		//we don't want to change the data from the staging buffer at this stage.
		//I made the assumption that the transformer process might need multiple samples from
		//the segment, therefore this staging buffer is just to give me extra space, to release 
		//the initial writer while I am doing the computations
		return RM_SUCCESS;
	}

	virtual RM_RETURN_CODE UseStagingMemory(char* pStagingSegmentMemory)
	{
		u_int uOriginalSegmentIndex = m_xStagingContext.GetCurrentWriteSegment();
		
		RM_SharedBuffer* pxSharedBuffer = m_xContext.GetDuplicateBuffer();
		if (!pxSharedBuffer) return RM_CUSTOM_ERR1;

		char* pOutputData = pxSharedBuffer->GetSegmentForWriting(uOriginalSegmentIndex);
		if (!pOutputData)
		{
			pxSharedBuffer->SetFinishedWriting(m_xStagingContext.GetWriteTag(), uOriginalSegmentIndex);
			return RM_CUSTOM_ERR2;
		}

		//transform elements
		for (u_int u = 0; u < SEGMENT_SIZE; ++u)
		{
			pOutputData[u] = ProcessElement(u, pStagingSegmentMemory);
		}
		printf(" Writing to segment %d [%c%c%c...] \n", uOriginalSegmentIndex, pOutputData[0], pOutputData[1], pOutputData[2]);

		pxSharedBuffer->SetFinishedWriting(m_xStagingContext.GetWriteTag(), uOriginalSegmentIndex);


		//send message to all processes that I've written to the duplicate buffer
		RM_WToRMessageData xMessage = { m_xStagingContext.GetWriteTag(), uOriginalSegmentIndex };
		RM_RETURN_CODE xResult = m_xContext.GetMessageManager()->SendMessage(-1, m_xContext.GetProcessIndex(), &xMessage);
		if (xResult != RM_SUCCESS)
		{
			//wait a little and try again for a few times (say 10 times, this param can be exposed)
			for (u_int u = 0; u < 10; ++u)
			{
				Sleep(5);
				xResult = m_xContext.GetMessageManager()->SendMessage(-1, m_xContext.GetProcessIndex(), &xMessage);
				if (xResult == RM_SUCCESS) break;
			}
			if (xResult != RM_SUCCESS)
			{
				printf("Failed to send message. The written data will never be read!");
				//TODO: could try looking for new readers, or we could increase buffer sizes - 
				//maybe the readers are too slow and fill up all the space?!
			}
		}

		return RM_SUCCESS;
	}


protected:
	virtual bool IsDerivedContextDirty() { return m_xContext.IsDirty(); }
	virtual void ResetDerivedContext(void* pDerivedContext)
	{
		Context* pxContext = reinterpret_cast<Context*>(pDerivedContext);
		m_xContext = *pxContext;
		m_xContext.ResetDirtyFlag();
	}

protected:
	Context m_xContext;
};

template<typename TStagingThread>
class RM_ThirdReader : public RM_StagingReader<TStagingThread>
{
public:
	RM_ThirdReader(int iProcessIndex, u_int uMaxNumRandomSegments, u_int uMaxNumPoolThreads, u_int uNumStagingSegments) :
		RM_StagingReader(iProcessIndex,uMaxNumRandomSegments,uMaxNumPoolThreads,uNumStagingSegments) 
	{
		RM_InitData xInitData;
		{
			RM_SharedMemory xSharedMemory;
			std::string xSharedInitDataName(SHARED_INIT_DATA_NAME);
			void* pSharedMemory = xSharedMemory.OpenMemory(RM_ACCESS_WRITE | RM_ACCESS_READ, SHARED_INIT_DATA_MAX_SIZE, xSharedInitDataName, m_iPID);
			RM_SharedInitDataLayout xInitDataLayout;
			xInitDataLayout.MapMemory(pSharedMemory);
			xInitData = xInitDataLayout.GetInitData(m_iProcessIndex);
		}

		//the duplicate buffer
		{
			RM_SharedMemory xSharedMemory;
			std::string xSharedBuffName(xInitData.pSharedBufferMemNameOut);
			void* pSharedMemory = xSharedMemory.OpenMemory(RM_ACCESS_WRITE, SHARED_MEMORY_MAX_SIZE, xSharedBuffName, m_iPID);
			
			RM_SharedMemory xSharedMemoryLabels;
			std::string xLablesObjName(xSharedBuffName);
			xLablesObjName.append("Lables");
			void* pSharedMemoryLabels = xSharedMemoryLabels.OpenMemory(RM_ACCESS_WRITE | RM_ACCESS_READ, SHARED_MEMORY_LABESLS_MAX_SIZE, xLablesObjName, m_iPID);

			m_xSharedBufferDuplicate.MapMemory(pSharedMemory, pSharedMemoryLabels);

			const int iPID = _getpid();
			std::string xMsgQName(xInitData.pMessageQueueNameOut);
			m_xMessageManagerDuplicate.Initialise(iPID, xMsgQName);
		}
	}

	virtual void ResetThreadContext(TStagingThread* pStagingThread, typename TStagingThread::StagingContext& xStagingContext)
	{
		TStagingThread::Context xDerivedContext(&m_xSharedBufferDuplicate, &m_xMessageManagerDuplicate, m_iProcessIndex);
		pStagingThread->ResetContexts(xStagingContext, &xDerivedContext);
	}

	virtual void CloseThreadContext() {};
	virtual RM_RETURN_CODE CustomReaderStartNewGroup() { return RM_SUCCESS; };
	virtual void EndIteration() {}

private:
	RM_SharedBuffer m_xSharedBufferDuplicate;

	//for commodity I will instance another message manager. In practice, it should merge with the first one, and handle
	//listening to messages fom a certain sender
	RM_MessageManager<RM_CHALLENGE_PROCESS_COUNT, RM_WToRMessageData> m_xMessageManagerDuplicate;
};

int main()
{
	//WaitKeyPress(3);
	RM_ThirdReader<RM_TR_ProcessingStagingThread> xReader(
		g_iProcessIndex, 
		READER_MAX_RANDOM_SEGMENTS, 
		READER_NUM_POOLTHREADS, 
		READER_NUM_BUFFERED_SEGMENTS);


	xReader.Initialise();

	xReader.ReadData();

	xReader.Shutdown();

	printf("WriterTransform done. Waiting key press... \n");
	WaitKeyPress(3);

	return S_OK;
}