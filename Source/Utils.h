#ifndef CHALLENGE_RM_UTILS
#define CHALLENGE_RM_UTILS

#include <atomic>

typedef unsigned int u_int;

struct SharedLabels
{
	std::atomic<int> m_iTimestamp;
	std::atomic<int> m_iWriteIndex;	//current segment to write
};

struct SegmentInfo
{
	std::atomic<int> m_iWriteTimestamp;
	std::atomic<int> m_iIsInUseForWriting;
	std::atomic<int> m_iIsInUseForReading;
	std::atomic<int> m_iWriteRequestSet;
	//volatile int m_iIsInUseForWriting;
	//volatile int m_iIsInUseForReading;
	//volatile int m_iWriteRequestSet;
};

#define SEGMENT_SIZE 1000000
#define NUM_SEGMENTS 10

#define SHARED_MEMORY_LABESLS_MAX_SIZE (sizeof(SharedLabels) + NUM_SEGMENTS * sizeof(SegmentInfo))
#define SHARED_MEMORY_LABESLS_NAME "NanoporeChallengeSharedMemoryLabesls"

#define SHARED_MEMORY_MAX_SIZE (sizeof(SharedLabels) + NUM_SEGMENTS * SEGMENT_SIZE)
#define SHARED_MEMORY_NAME "NanoporeChallengeSharedMemory"
#define CHALLENGE_EVENT L"ChallengeEvent"




enum RM_ACCESS_FLAG
{
	RM_ACCESS_NO_ACCESS = 0,
	RM_ACCESS_READ = 1,
	RM_ACCESS_WRITE = 2,
};

enum RM_RETURN_CODE
{
	RM_SUCCESS,
	RM_CUSTOM_ERR1,
	RM_CUSTOM_ERR2,
	RM_CUSTOM_ERR3,//..
};

static void WaitKeyPress(int iKey);

#endif//CHALLENGE_RM_UTILS