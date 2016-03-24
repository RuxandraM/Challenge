#ifndef CHALLENGE_RM_STAGING_BUFFER
#define CHALLENGE_RM_STAGING_BUFFER

#include "../Source/Utils.h"
#include "../Source/RM_Mutex.h"

class RM_StagingSegment
{
public:
	RM_StagingSegment():m_pMemory(nullptr),
		m_bInUseForReading(false),
		m_bInUseForWriting(false)
	{}

	RM_RETURN_CODE Initialise()
	{
		m_pMemory = new char[SEGMENT_SIZE];
		if (!m_pMemory) { return RM_CUSTOM_ERR1; }
		return RM_SUCCESS;
	}

	//method not thread safe!!
	bool IsAvailable() const
	{
		return !m_bInUseForReading && !m_bInUseForWriting;
	}

	void ReserveForWriting() { m_bInUseForWriting = true; }
	void EndWritingBeginReading()
	{
		m_bInUseForWriting = false;
		m_bInUseForReading = true;
	}

	void EndReading()
	{
		m_bInUseForWriting = false;
		m_bInUseForReading = false;
	}

	char* GetMemory() { return m_pMemory; }

private:
	std::atomic<bool> m_bInUseForReading;
	std::atomic<bool> m_bInUseForWriting;
	char* m_pMemory;
};


class RM_StagingBuffer
{
public:

	RM_StagingBuffer() :m_uNumStagingSegments(0), m_pSegments(nullptr) {}
	~RM_StagingBuffer() { delete[] m_pSegments; }

	RM_RETURN_CODE Initialise(u_int uNumStagingSegments)
	{
		m_uNumStagingSegments = uNumStagingSegments;
		m_pSegments = new RM_StagingSegment[m_uNumStagingSegments];

		for (u_int u = 0; u < m_uNumStagingSegments; ++u)
		{
			if (m_pSegments[u].Initialise() != RM_SUCCESS)
				return RM_CUSTOM_ERR1;
		}
		return RM_SUCCESS;
	}


	//this function is not thread safe!! I'm assuming the segment sync logic was handled by now
	char* GetSegmentForWriting(int iSegmentIndex)
	{
		return m_pSegments[iSegmentIndex].GetMemory();
	}

	int ReserveSegmentIfAvailable()
	{
		for (u_int u = 0; u < m_uNumStagingSegments; ++u)
		{
			RM_StagingSegment& xSegment = m_pSegments[u];
			bool bFound = false;
			m_xFlagMutex.Lock();
			if (xSegment.IsAvailable())
			{
				xSegment.ReserveForWriting();
				bFound = true;
			}
			m_xFlagMutex.Unlock();

			if (bFound)
			{
				return u;
			}
		}
		return -1;
	}

	void EndWritingBeginReadingToSegment(int iIndex)
	{
		RM_StagingSegment& xSegment = m_pSegments[iIndex];
		m_xFlagMutex.Lock();
		xSegment.EndWritingBeginReading();
		m_xFlagMutex.Unlock();
	}

	void EndReadingFromSegment(int iIndex)
	{
		RM_StagingSegment& xSegment = m_pSegments[iIndex];
		m_xFlagMutex.Lock();
		xSegment.EndReading();
		m_xFlagMutex.Unlock();
	}

private:
	u_int m_uNumStagingSegments;
	RM_StagingSegment* m_pSegments;
	RM_PlatformMutex m_xFlagMutex;
};


#endif//CHALLENGE_RM_STAGING_BUFFER