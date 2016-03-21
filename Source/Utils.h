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
#define CHALLENGE_EVENT_NAME "ChallengeEvent"


enum RM_CHALLENGE_PROCESS
{
	RM_CHALLENGE_PROCESS_WRITER,
	RM_CHALLENGE_PROCESS_FIRSTREADER,
	RM_CHALLENGE_PROCESS_SECONDREADER,
	RM_CHALLENGE_PROCESS_COUNT
};

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

static void itostr(int iNumber, char buf[33])
{
	//char c = 0x0;
	//int sign = 0;
	//int i, end, top;
	//end = top = 32;
	u_int uCurrentDigit = 0;
	char *pCurrentDigit = buf;
	if (iNumber < 0) 
	{
		buf[0] = '-';
		iNumber *= -1;
		++pCurrentDigit;
	}

	if (iNumber == 0)
	{
		buf[0] = '0';
		buf[1] = '\0';
		return;
	}

	//TODO: it is reveresed, but it doesn't matter here
	while (iNumber)
	{
		int iDigit = iNumber % 10;
		*pCurrentDigit++ = (iDigit + '0');
		++uCurrentDigit;
		iNumber /= 10;
	}
	buf[uCurrentDigit] = '\0';

	//for (i = x; i != 0; i /= 10) {
	//	c |= 0x03;
	//	c <<= 4;
	//	c |= i % 10;
	//	buf[top--] = c;
	//	c = 0x0;
	//}
	//if (sign)
	//	buf[0] = '-';
	//for (i = sign; top < end; i++)
	//	buf[i] = buf[++top];
	//buf[i] = '\0';
	
	return;
}

#endif//CHALLENGE_RM_UTILS