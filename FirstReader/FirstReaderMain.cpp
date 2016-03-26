#include <stdio.h>
#include <windows.h>
#include "../Source/Utils.h"
#include "../Source/RM_SharedBuffer.h"
#include "../Source/RM_SharedMemory.h"
#include "../Source/RM_MessageManager.h"
#include "../Source/RM_ReaderProcess.h"


static int g_iProcessIndex = 1; //TODO: this is hardcoded; it should be sent at creation



class RM_FirstReader : public RM_ReaderProcess
{
public:

	RM_FirstReader(int iProcessIndex) :RM_ReaderProcess(iProcessIndex), m_uLastSegmentRead(0)
	{}

	RM_RETURN_CODE ReadData()
	{
		while (1)
		{
			RM_WToRMessageData xMessage;
			RM_RETURN_CODE xResult = m_xMessageManager.BlockingReceiveWithFlush(g_iProcessIndex, &xMessage);
			if (xResult != RM_SUCCESS)
			{
				printf("[%d] Failed to read signalled message.", m_iPID);
				continue;	//go back and wait for other messages. It was a problem in the transmition medium
			}

			printf("[%d] Received signal for segment %d \n", m_iPID, xMessage.m_uSegmentWritten);

			if (xMessage.m_uTag - m_uLastTag >= NUM_SEGMENTS)
			{
				//we lost data. The writer was too quick and I wasn't listening for new events because I was busy writing out data
				//attempt to read all the data from the buffer
				m_uLastSegmentRead = (xMessage.m_uSegmentWritten + 1) % NUM_SEGMENTS;
			}

			//I was signalled that there is some data to read
			u_int uCurrentSegment = m_uLastSegmentRead;
			u_int uMaxNumIterations = xMessage.m_uSegmentWritten >= m_uLastSegmentRead ?
				xMessage.m_uSegmentWritten - m_uLastSegmentRead :
				NUM_SEGMENTS - m_uLastSegmentRead + xMessage.m_uSegmentWritten;

			//read all the segments from the last one I read to the one I was messaged about,
			//or the last N if the writer is too far ahead
			for (u_int u = 0; u < uMaxNumIterations; ++u)
			{
				--uMaxNumIterations;
				u_int uSegmentTag = (u_int)m_xSharedBuffer.GetSegmentTimestamp(uCurrentSegment);
				//TODO - the writer will reset the u_int at some point
				//if (uSegmentTag < m_uLastTag)
				//{
				//	break;
				//}

				//---------------------------------------------------------
				const char* pSegmentMemory = m_xSharedBuffer.GetSegmentForReading(uCurrentSegment);

				if (!pSegmentMemory)
				{
					//Data Lost because the writer was too fast
					printf("First Reader error - skips data for segment %d \n", uCurrentSegment);
				}
				else
				{
					//I grabbed the pointer to the segment I want to read! Spawn a thread that copies this to an HDF5

					char szBuffer[10];
					memcpy(szBuffer, pSegmentMemory, 9);
					m_xSharedBuffer.SetFinishedReading(uCurrentSegment);
					szBuffer[9] = '\0';
					
					printf("First Reader: %s \n", szBuffer);
				}
				//---------------------------------------------------------

				uCurrentSegment = (uCurrentSegment + 1) % NUM_SEGMENTS;
			}
			
			m_uLastTag = xMessage.m_uTag;
			m_uLastSegmentRead = uCurrentSegment;
		}
	}
private:
	u_int m_uLastSegmentRead;
	u_int m_uLastTag;
};

int main()
{
	RM_FirstReader xReader(g_iProcessIndex);
	xReader.Initialise();

	xReader.ReadData();
	
	xReader.Shutdown();

	printf("First Reader done. Waiting key press to start... \n");
	system("pause");

	return S_OK;
}