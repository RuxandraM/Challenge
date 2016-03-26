#ifndef CHALLENGE_RM_UTILS
#define CHALLENGE_RM_UTILS


typedef unsigned int u_int;

#define DEBUG_OUTPUT_TO_CONSOLE 1
#define CHALLENGE_OPEN_FILE_DELAY_IN_NANOSEC 400
#define CHALLENGE_WRITE_FILE_DELAY_IN_NANOSEC 2000



#include "RM_Mutex.h"
#include <atomic>


struct SharedLabels
{
	std::atomic<int> m_iTimestamp;
	std::atomic<int> m_iWriteIndex;	//current segment to write
};

struct SegmentInfo
{
	SegmentInfo() :m_iWriteTimestamp(0) {}

	std::atomic<int> m_iWriteTimestamp;
	RM_ReadWriteLogic m_xRWLogic;
};

struct RM_InitData
{
	RM_InitData()
	{
		memset(this, 0, sizeof(RM_InitData));
	}

	u_int m_uNumWriteIterations;
	char pMessageQueueNameIn[128];
	char pSharedBufferMemNameIn[128];
	char pSharedLabelsMemNameIn[128];
	char pMessageQueueNameOut[128];
	char pSharedBufferMemNameOut[128];
	char pSharedLabelsMemNameOut[128];
	char pOutputFileName[512];
};


#define SEGMENT_SIZE 1000000
#define NUM_SEGMENTS 10

enum RM_CHALLENGE_PROCESS
{
	RM_CHALLENGE_PROCESS_WRITER,
	RM_CHALLENGE_PROCESS_FIRSTREADER,
	RM_CHALLENGE_PROCESS_SECONDREADER,
	RM_CHALLENGE_PROCESS_WRITER_TRANSFORM,
	RM_CHALLENGE_PROCESS_READER_TRANSFORM,
	RM_CHALLENGE_PROCESS_COUNT
};

#define SHARED_INIT_DATA_MAX_SIZE (sizeof(RM_InitData) * RM_CHALLENGE_PROCESS_COUNT)
#define SHARED_INIT_DATA_NAME "GlobalChallengeInitData"

#define SHARED_MEMORY_LABESLS_MAX_SIZE (sizeof(SharedLabels) + NUM_SEGMENTS * sizeof(SegmentInfo))

#define SHARED_MEMORY_MAX_SIZE (sizeof(SharedLabels) + NUM_SEGMENTS * SEGMENT_SIZE)
#define CHALLENGE_EVENT_NAME "ChallengeEvent"




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


//the layout of the messages between the writer process and the reader ones
struct RM_WToRMessageData
{
	u_int m_uTag;				//every time the producer writes a new segment it assigns it a unique tag
	u_int m_uSegmentWritten;	//the segment that has just been written by the producer
};

struct FileLimits
{
	u_int m_uNumFiles;
	u_int m_uNumSegmentsPerFile;
	u_int m_uNumTotalSegments;
};



static FileLimits GetFileLimits_Custom(u_int uTotalNumSegments, u_int READER_NUM_SEGMENTS_FILE, u_int READER_MAX_NUM_FILES, u_int READER_MAX_NUM_SEGMENTS_FILE)
{
	FileLimits xResult;

	xResult.m_uNumTotalSegments = uTotalNumSegments;

	//ideally we want READER_NUM_SEGMENTS_FILE segments in each
	xResult.m_uNumFiles = (uTotalNumSegments + READER_NUM_SEGMENTS_FILE - 1) / READER_NUM_SEGMENTS_FILE;
	if (xResult.m_uNumFiles == 0)
	{
		xResult.m_uNumFiles = 1;
		xResult.m_uNumSegmentsPerFile = uTotalNumSegments;
		return xResult;
	}

	//last file may have less lines
	xResult.m_uNumSegmentsPerFile = READER_NUM_SEGMENTS_FILE;

	if (xResult.m_uNumFiles > READER_MAX_NUM_FILES)
	{
		xResult.m_uNumFiles = READER_MAX_NUM_FILES;
		xResult.m_uNumSegmentsPerFile = (uTotalNumSegments + READER_MAX_NUM_FILES - 1) / READER_MAX_NUM_FILES;
		if (xResult.m_uNumSegmentsPerFile > READER_MAX_NUM_SEGMENTS_FILE)
		{
			xResult.m_uNumSegmentsPerFile = READER_MAX_NUM_SEGMENTS_FILE;
			xResult.m_uNumTotalSegments = READER_MAX_NUM_SEGMENTS_FILE * READER_MAX_NUM_FILES;
		}
	}
	return xResult;
}

#endif//CHALLENGE_RM_UTILS