#ifndef CHALLENGE_THIRD_READER_H_
#define CHALLENGE_THIRD_READER_H_

#define READER_MAX_RANDOM_SEGMENTS 20	//how many consecutive segments to read (max num)
#define READER_NUM_BUFFERED_SEGMENTS 5	//how much space (in segments) we want to use for double buffering
#define READER_NUM_POOLTHREADS 5		//how many threads do we want to use for outputting to file. 
//If this is larger than READER_NUM_BUFFERED_SEGMENTS it means we want to
//write out some files without double-buffering (not implemented yet)
#define READER_NUM_SEGMENTS_FILE 3		//this actually tells how many threads will be waiting for each other to gain access to the file
//If this is larger than READER_NUM_POOLTHREADS all the threads will be waiting in a chain
#define READER_MAX_NUM_SEGMENTS_FILE 7	//Max limit in case we want to read too much data, to prevent the num files exceeding READER_MAX_NUM_FILES
#define READER_MAX_NUM_FILES 10			//Max total number of files allowed:
//We would need to write at most READER_MAX_RANDOM_SEGMENTS with READER_NUM_SEGMENTS_FILE per file.
//If the total number of files exceeds this, clamp the range and put 
//min(READER_MAX_NUM_SEGMENTS_FILE, READER_MAX_RANDOM_SEGMENTS/READER_MAX_NUM_FILES). 

//#define READER_OUTPUT_NAME	"ThirdReaderOut"

#endif//CHALLENGE_THIRD_READER_H_