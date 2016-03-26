1. Launching and testing

In the folder _ReleaseAndTest I've put the executables and some in test files, though not all the parameters are exposed to the input file unfortunately.
The input file can decide how many iterations to write, and customises a few shared resources (communication channels).

Run with 
NanoporeChallenge.exe test1.txt

The results will be in the same folder, the second and last reader are outputting to files: SecondReader_GroupIndex_FileIndex and ReaderTransform respectively.
The writer outputs rows of consecutive characters, the transform offsets them and returns another row of consecutive characters.

The first reader is outputting to console only. HDF5 functionality was not implemented yet, so I'm outputting the start of the segments to an fstream. However, there is an option to delay the threads during writing to file/opening file, but it is in c++ code only at the moment (see 

CHALLENGE_OPEN_FILE_DELAY_IN_NANOSEC, CHALLENGE_WRITE_FILE_DELAY_IN_NANOSEC).

Some more feedback is rendered at the console for each process. After finishing writing, the readers are sleeping, waiting for events. Close by closing the initial console.


More defines can be adjusted in cpp code, not yet exposed, see SecondReader.h and ReaderTransform.h for
READER_MAX_RANDOM_SEGMENTS, READER_NUM_SEGMENTS_FILE, READER_NUM_BUFFERED_SEGMENTS, READER_MAX_NUM_FILES etc.

(Tested on Win10 using VS 140 toolset chain, 64bits)


2. Description



The writer is the one who starts to produce data (a segment every second, but in reality is actually sleeping for 1 second in between iterations - it should respond to signals that are sent at an exact deltaT). When the data is written it sends events to all the other processes with the index of the segment that has been written, and a tag that counts the iterations).

First reader - is waken up by the event 'data was written'. This has a backwards approach - it starts reading on its main thread and (eventually - not implemented) writing to output file. At the next event it will try to process all the segments from the last read. There are lots of issues with this approach - if the reader is too slow, it can only get slower, it is prone to loosing segments as the writer goes around the circular buffer, keeps a segment locked for reading throughout the whole write to file operation.

Second reader - it wakes up at the writer's events, and spawns a new thread that needs to handle the given segment. It has a staging segments pool, and at every event, it will give the new thread a segment from the staging pool. The thread is meant to copy the original segment to the staging buffer, and output that to file. The output can be to multiple files, and access to any given file is serial. We know we want to read n segments, that will be evenly spread in between m files. The files will have a set of threads that can write to them in a serial manner (in a pass the token fashion).

'WriterTransform' - it is implemented in the same way as the second reader, with staging buffers and worker threads. But the workers are not writing to file, and not passing the token. They are writing to a designated area of the main buffer copy.

'ReaderTransform' - implemented exactly the same as second reader, it only listens to 'WriterTransform' instead of 'Writer'


Note that in this design we never stop the writer in a lock. If it comes across a segment that is in use for reading it will just print an error message. The double-buffering from the staging segments should allow the writer to move on. If it is still to quick, a decision must be made - either make the writer wait, or increase the buffer/staging buffer limits.


3. Implementation


There is a hierarchy of reader processes:

RM_ReaderProcess - top of the hierarchy, abstract class. It 	initialises shared data and communication channels. 

RM_StagingReader<StagingThread> - abstract class, it is dealing 	with reading to the staging buffer logic, and creates the 	worker threads that will do the copy. The template 	StagingThread must be one of RM_OutputStagingThread.

RM_SecondReader<StagingThread> - derives from the above, and the thread must also be one of RM_OutputStagingThread, which derives from the template argument above. This reader handles the file output logic - which staging thread to write to which file. RM_OutputStagingThread must write to a given file and pass the token to the rest when it's done.

RM_SecondReader<RM_SR_OutputStagingThread> is the concrete object used by the second reader and the 'ReaderTransform'. Basically, this means - spawn a thread that will take the current segment and copy it to the staging buffer -> this is the logic from RM_StagingReader<StagingThread>. But I also want this thread to wite to file, and I want to prepare the context for it - hence the logic from RM_SecondReader<StagingThread>.

RM_ThirdReader<StagingThread> is not adding more functionality than RM_SecondReader<StagingThread>, but it provides a custom context for its threads (that will let them know where in the second buffer to write)

RM_ThirdReader<RM_TR_ProcessingStagingThread> is the concrete object that reads from the first shared buffer and outputs to the second.


RM_OutputStagingThread (abstract) - is meant to copy segments to staging and call a pure virtual to process it. It can be RM_SR_OutputStagingThread (which outputs to file) or RM_TR_ProcessingStagingThread (which outputs to a second buffer).