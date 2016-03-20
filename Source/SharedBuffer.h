#ifndef CHALLENGE_RM_SHARED_BUFFER
#define CHALLENGE_RM_SHARED_BUFFER

#include "Utils.h"
#include <atomic>
#include <signal.h>

class Segment
{
public:
	Segment() : m_pSegmentInfo(nullptr), m_pMemory(nullptr)
	{}

	~Segment() {}

	void Initialise(char* pMemory, char* pLabelMemory)
	{
		m_pSegmentInfo = reinterpret_cast<SegmentInfo*>(pLabelMemory);
		m_pMemory = pMemory;
	}

	void Shutdown() {}

	void ClearFlags()
	{
		//atomic operator = (see definition)
		m_pSegmentInfo->m_iWriteTimestamp = 0;
		m_pSegmentInfo->m_iWriteRequestSet = 0;
		m_pSegmentInfo->m_iIsInUseForWriting = 0;
		m_pSegmentInfo->m_iWriteRequestSet = 0;
	}

	static long long GetMemoryBufferSize() { return SEGMENT_SIZE; }
	static long long GetLabelSize() { return sizeof(SegmentInfo); }

	bool AddWriteRequest()
	{
		m_pSegmentInfo->m_iWriteRequestSet = 1u;
		//GUGU: let the others know I want to write here

		if (m_pSegmentInfo->m_iIsInUseForReading)
		{	
			return false;	//people are still reading from here
		}
		return true;	//write request added succesfully, no other processes are reading
	}


	char* GetMemoryForWriting()
	{
		//GUGU: this time is not relevant between different machines
		//m_pSegmentInfo->m_iWriteTimestamp = GetTickCount();
		m_pSegmentInfo->m_iIsInUseForWriting = 1;
		return m_pMemory;
	}

	const char* GetMemoryForReading()
	{
		m_pSegmentInfo->m_iIsInUseForReading = 1;
		return m_pMemory;
	}

	void SetFinishedReading()
	{
		m_pSegmentInfo->m_iIsInUseForReading = 0;
	}

	void SetFinishedWriting(int iTimestamp)
	{
		m_pSegmentInfo->m_iWriteRequestSet = 0;
		m_pSegmentInfo->m_iIsInUseForWriting = 0;
		m_pSegmentInfo->m_iWriteTimestamp = iTimestamp;
	}

	bool IsInUseForWriting()
	{
		return (m_pSegmentInfo->m_iWriteRequestSet != 0) || (m_pSegmentInfo->m_iIsInUseForWriting != 0);
	}

	int GetTimestamp() const
	{
		return m_pSegmentInfo->m_iWriteTimestamp;
	}

private:
	SegmentInfo* m_pSegmentInfo;
	char* m_pMemory;
};

class SharedBuffer
{
public:
	SharedBuffer(void* pMemory, void* pLabelsMemory) : m_pxLables(nullptr), m_pSegments(nullptr)
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
			pSeg->Initialise(pCurrentMemPtr, pCurrentLabelPtr);
			pCurrentMemPtr += Segment::GetMemoryBufferSize();
			pCurrentLabelPtr += Segment::GetLabelSize();
			++pSeg;
		}
	}

	~SharedBuffer() { Shutdown(); }

	void ClearFlags()
	{
		m_pxLables->m_iWriteIndex = 0;
		m_pxLables->m_iTimestamp = 0;
		if (!m_pSegments) return;
		
		Segment* pSeg = m_pSegments;
		while ((m_pSegments + NUM_SEGMENTS) - pSeg > 0)
		{
			pSeg->ClearFlags();
			pSeg++;
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

	void SetFinishedWriting(u_int uSegmentIndex, int iTimestamp)
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
		if (pCurrentSegment->IsInUseForWriting()) return nullptr;
		return pCurrentSegment->GetMemoryForReading();
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