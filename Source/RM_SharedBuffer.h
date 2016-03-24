#ifndef CHALLENGE_RM_SHARED_BUFFER
#define CHALLENGE_RM_SHARED_BUFFER

#include "Utils.h"
#include "RM_Mutex.h"
#include <atomic>
#include <signal.h>

class Segment
{
public:
	Segment() : m_pSegmentInfo(nullptr), m_pMemory(nullptr)
	{}

	~Segment() {}

	void MapMemory(char* pMemory, char* pLabelMemory)
	{
		m_pSegmentInfo = reinterpret_cast<SegmentInfo*>(pLabelMemory);
		m_pMemory = pMemory;
	}

	//assumes it has previously been mapped
	void CreateWitoutMap()
	{
		//placement new will call the constructor for SegmentInfo for the memory in m_pSegmentInfo - which has been already mapped to the shared memory
		//operator new (SegmentInfo,&m_pSegmentInfo)
		new (m_pSegmentInfo) SegmentInfo;
	}

	void Shutdown() {}

	//void ClearFlags()
	//{
		//atomic operator = (see definition)
		//m_pSegmentInfo->m_iWriteTimestamp = 0;
		//m_pSegmentInfo->m_iWriteRequestSet = 0;
		//m_pSegmentInfo->m_iIsInUseForWriting = 0;
		//m_pSegmentInfo->m_iWriteRequestSet = 0;
	//}

	static long long GetMemoryBufferSize() { return SEGMENT_SIZE; }
	static long long GetLabelSize() { return sizeof(SegmentInfo); }

	bool AddWriteRequest()
	{
		return m_pSegmentInfo->m_xRWLogic.AddWriteRequest();

		//If it hasn't succeded it means the segment is still in use by readers; 
		//TODO: what to do here? I don't want to delay the writer
		//if (!bReserveSucceded){ return false; }

		//m_pSegmentInfo->m_iWriteRequestSet = 1u;
		//GUGU: let the others know I want to write here

		//if (m_pSegmentInfo->m_iIsInUseForReading)
		//{	
		//	return false;	//people are still reading from here
		//}
		//return true;	//write request added succesfully, no other processes are reading
	}

	bool AddReadRequest()
	{
		return m_pSegmentInfo->m_xRWLogic.AddReadRequest();
	}

	char* GetMemoryForWriting()
	{
		//GUGU: this time is not relevant between different machines
		//m_pSegmentInfo->m_iWriteTimestamp = GetTickCount();
		//m_pSegmentInfo->m_iIsInUseForWriting = 1;
		return m_pMemory;
	}

	const char* GetMemoryForReading()
	{
		//m_pSegmentInfo->m_iIsInUseForReading = 1;
		//m_pSegmentInfo->m_xRWLogic.ReserveForReading();
		return m_pMemory;
	}

	void SetFinishedReading()
	{
		m_pSegmentInfo->m_xRWLogic.SetFinishedReading();
	}

	void SetFinishedWriting(int iTimestamp)
	{
		m_pSegmentInfo->m_xRWLogic.SetFinishedWriting();
		//m_pSegmentInfo->m_iWriteRequestSet = 0;
		//m_pSegmentInfo->m_iIsInUseForWriting = 0;
		m_pSegmentInfo->m_iWriteTimestamp = iTimestamp;
	}

	//bool IsInUseForWriting()
	//{
	//	return (m_pSegmentInfo->m_iWriteRequestSet != 0) || (m_pSegmentInfo->m_iIsInUseForWriting != 0);
	//}

	int GetTimestamp() const
	{
		return m_pSegmentInfo->m_iWriteTimestamp;
	}

private:
	SegmentInfo* m_pSegmentInfo;
	char* m_pMemory;
};

class RM_SharedBuffer
{
public:

	RM_SharedBuffer() :m_pSegments(nullptr), m_pxLables(nullptr) {}
	~RM_SharedBuffer() { delete[] m_pSegments; }

	void MapMemory(void* pMemory, void* pLabelsMemory)
	{
		//map pMemory and pLabelsMemory to the layout of this class
		m_pxLables = reinterpret_cast<SharedLabels*>(pLabelsMemory);

		//offset by shared labels
		char* pCurrentMemPtr = reinterpret_cast<char*>(pMemory);
		char* pCurrentLabelPtr = reinterpret_cast<char*>(pLabelsMemory) + sizeof(SharedLabels);

		m_pSegments = new Segment[NUM_SEGMENTS];

		Segment* pSeg = m_pSegments;
		//loop through all the segments to initialise them
		while ((m_pSegments + NUM_SEGMENTS) - pSeg > 0)
		{
			pSeg->MapMemory(pCurrentMemPtr, pCurrentLabelPtr);
			pCurrentMemPtr += Segment::GetMemoryBufferSize();
			pCurrentLabelPtr += Segment::GetLabelSize();
			++pSeg;
		}
	}

	void Create(void* pMemory, void* pLabelsMemory)
	{
		MapMemory(pMemory, pLabelsMemory);

		m_pxLables->m_iWriteIndex = 0;
		m_pxLables->m_iTimestamp = 0;

		////map pMemory and pLabelsMemory to the layout of this class
		//m_pxLables = reinterpret_cast<SharedLabels*>(pLabelsMemory);
		//
		////offset by shared labels
		//char* pCurrentMemPtr = reinterpret_cast<char*>(pMemory);
		//char* pCurrentLabelPtr = reinterpret_cast<char*>(pLabelsMemory) + sizeof(SharedLabels);

		Segment* pSeg = m_pSegments;
		//loop through all the segments to initialise them
		while ((m_pSegments + NUM_SEGMENTS) - pSeg > 0)
		{
			pSeg->CreateWitoutMap();
			++pSeg;
		}
	}
	

	char* GetSegmentForWriting(u_int uSegmentIndex)
	{
		Segment* pCurrentSegment = &m_pSegments[uSegmentIndex];
		if (pCurrentSegment->AddWriteRequest())
		{
			return pCurrentSegment->GetMemoryForWriting();
		}
		else
		{
			//segment in use for reading - GUGU - todo
			return nullptr;
		}
	}

	void SetFinishedWriting( int iTimestamp, u_int uSegmentIndex )
	{
		m_pxLables->m_iTimestamp = iTimestamp;
		m_pxLables->m_iWriteIndex = uSegmentIndex;
		Segment* pCurrentSegment = &m_pSegments[uSegmentIndex];
		pCurrentSegment->SetFinishedWriting(iTimestamp);
	}

	void SetFinishedReading(u_int uSegmentIndex)
	{
		m_pSegments[uSegmentIndex].SetFinishedReading();
	}

	const char* GetSegmentForReading(u_int uSegmentIndex)
	{
		//GUGU atomic!
		Segment* pCurrentSegment = &m_pSegments[uSegmentIndex];
		if (pCurrentSegment->AddReadRequest())
		{
			return pCurrentSegment->GetMemoryForReading();
		}
		return nullptr;

		//if (pCurrentSegment->IsInUseForWriting()) return nullptr;
		//return pCurrentSegment->GetMemoryForReading();
	}

	int GetSegmentTimestamp(u_int uSegmentIndex)
	{
		return m_pSegments[uSegmentIndex].GetTimestamp();
	}

	int GetTimestamp() const
	{
		return m_pxLables->m_iTimestamp;
	}
	int GetLastSegmentWrittenIndex() const
	{
		return m_pxLables->m_iWriteIndex;
	}

	//void Initialise(u_int uSizeInSegments = 10u, u_int uSegmentSize = 1000000u)
	//{
	//	m_pSegments = new Segment[uSizeInSegments];
	//	Segment *pSeg = m_pSegments;
	//	while ((m_pSegments + uSizeInSegments) - pSeg > 0 )
	//	{
	//		pSeg->Initialise(uSegmentSize);
	//		++pSeg;
	//	}
	//}

	void Shutdown()
	{
		delete [] m_pSegments;
		m_pSegments = nullptr;
	}

private:
	

	SharedLabels* m_pxLables;
	Segment* m_pSegments;
};

#endif//CHALLENGE_RM_SHARED_BUFFER