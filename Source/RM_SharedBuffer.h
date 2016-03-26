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

	static long long GetMemoryBufferSize() { return SEGMENT_SIZE; }
	static long long GetLabelSize() { return sizeof(SegmentInfo); }

	bool AddWriteRequest()
	{
		return m_pSegmentInfo->m_xRWLogic.AddWriteRequest();
	}

	bool AddReadRequest()
	{
		return m_pSegmentInfo->m_xRWLogic.AddReadRequest();
	}

	char* GetMemoryForWriting()
	{
		return m_pMemory;
	}

	const char* GetMemoryForReading()
	{
		return m_pMemory;
	}

	void SetFinishedReading()
	{
		m_pSegmentInfo->m_xRWLogic.SetFinishedReading();
	}

	void SetFinishedWriting(int iTimestamp)
	{
		m_pSegmentInfo->m_xRWLogic.SetFinishedWriting();
		m_pSegmentInfo->m_iWriteTimestamp = iTimestamp;
	}

	int GetTimestamp() const
	{
		return m_pSegmentInfo->m_iWriteTimestamp;
	}

private:
	SegmentInfo* m_pSegmentInfo;
	char* m_pMemory;
};



//holds a list of segments (shared memory). Labels is a shared RW memory used by all the processes
//to synchronise the access to the segments (is in use for read/write etc)
class RM_SharedBuffer
{
public:

	RM_SharedBuffer() :m_pSegments(nullptr), m_pxLables(nullptr) {}
	~RM_SharedBuffer() { delete[] m_pSegments; }

	//map memory is reinterpreting the memory to set where my data is pointing to, as opposed to create, that calls the constructors
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

	//maps the memory and calls the constructors to initialise it. This is done once, before the processes are created and it becomes shared.
	void Create(void* pMemory, void* pLabelsMemory)
	{
		MapMemory(pMemory, pLabelsMemory);

		m_pxLables->m_iWriteIndex = 0;
		m_pxLables->m_iTimestamp = 0;

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
			//segment in use for reading
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
		Segment* pCurrentSegment = &m_pSegments[uSegmentIndex];
		if (pCurrentSegment->AddReadRequest())
		{
			return pCurrentSegment->GetMemoryForReading();
		}
		return nullptr;
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