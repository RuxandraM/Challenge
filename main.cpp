#include <stdio.h>
#include "Source/Utils.h"
#include "Source/RM_SharedBuffer.h"
#include "Source/RM_Process.h"
#include "Source/RM_SharedMemory.h"
#include "Source/RM_MessageManager.h"
#include "Source/RM_SharedInitData.h"
#include <time.h> 
#include <fstream>
#include <vector>

static u_int g_iPID = 0;

struct GlobalStrings
{
	std::vector< std::string > m_xMessageQueues;
	std::vector< std::string > m_xSharedBuffers;
};

void RM_ParseInput(std::fstream& xInFileStream, RM_SharedInitDataLayout& xInitDataLayout, GlobalStrings& xGlobalStrings)
{
	int iCurrentProcessIndex = -1;
	RM_InitData* pxCurrentData = nullptr;
	while (!xInFileStream.eof())
	{
		std::string xValName;
		xInFileStream >> xValName;

		if (xValName.compare("<SharedPoolNames>") == 0)
		{
			while (xValName.compare("</SharedPoolNames>") != 0)
			{
				xInFileStream >> xValName;
				if (xValName.compare("MessageQueue") == 0)
				{
					xInFileStream >> xValName;
					xGlobalStrings.m_xMessageQueues.push_back(xValName);
				}
				else if (xValName.compare("SharedBuffer") == 0)
				{
					xInFileStream >> xValName;
					xGlobalStrings.m_xSharedBuffers.push_back(xValName);
				}
			}
		}
		else if (xValName.compare("//----------------------") == 0)
		{
			//new process
			++iCurrentProcessIndex;
			pxCurrentData = xInitDataLayout.GetInitDataPtr(iCurrentProcessIndex);
		}
		else if (xValName.compare("MessageQueueOut") == 0)
		{
			xInFileStream >> xValName;
			u_int uLen = (u_int)min(xValName.length(), 127u);
			memcpy(pxCurrentData->pMessageQueueNameOut, xValName.c_str(), uLen);
			pxCurrentData->pMessageQueueNameOut[uLen] = '\0';
		}
		else if (xValName.compare("MessageQueueIn") == 0)
		{
			xInFileStream >> xValName;
			u_int uLen = (u_int)min(xValName.length(), 127u);
			memcpy(pxCurrentData->pMessageQueueNameIn, xValName.c_str(), uLen);
			pxCurrentData->pMessageQueueNameIn[uLen] = '\0';
		}
		else if (xValName.compare("SharedBufferIn") == 0)
		{
			xInFileStream >> xValName;
			u_int uLen = (u_int)min(xValName.length(), 127u);
			memcpy(pxCurrentData->pSharedBufferMemNameIn, xValName.c_str(), uLen);
			pxCurrentData->pSharedBufferMemNameIn[uLen] = '\0';
			memcpy(pxCurrentData->pSharedLabelsMemNameIn, pxCurrentData->pSharedBufferMemNameIn, uLen + 1);
		}
		else if (xValName.compare("SharedBufferOut") == 0)
		{
			xInFileStream >> xValName;
			u_int uLen = (u_int)min(xValName.length(), 127u);
			memcpy(pxCurrentData->pSharedBufferMemNameOut, xValName.c_str(), uLen);
			pxCurrentData->pSharedBufferMemNameOut[uLen] = '\0';
			memcpy(pxCurrentData->pSharedLabelsMemNameOut, pxCurrentData->pSharedBufferMemNameOut, uLen + 1);
		}
		else if (xValName.compare("OutputFileName") == 0)
		{
			xInFileStream >> xValName;
			u_int uLen = (u_int)min(xValName.length(), 511u);
			memcpy(pxCurrentData->pOutputFileName, xValName.c_str(), uLen);
			pxCurrentData->pOutputFileName[uLen] = '\0';
		}
		else if (xValName.compare("NumWriteIterations") == 0)
		{
			xInFileStream >> pxCurrentData->m_uNumWriteIterations;
		}
	}
}

int main(int argc, char *argv[])
{
	g_iPID = _getpid();

	// initialize random seed:
	srand(time(NULL));

	RM_SharedInitDataLayout xInitDataLayout;
	{
		RM_SharedMemory xSharedMemory;
		std::string xSharedInitDataName(SHARED_INIT_DATA_NAME);
		xSharedMemory.Create(RM_ACCESS_WRITE | RM_ACCESS_READ, SHARED_INIT_DATA_MAX_SIZE, xSharedInitDataName, g_iPID);
		void* pSharedMemory = xSharedMemory.OpenMemory(RM_ACCESS_WRITE | RM_ACCESS_READ, SHARED_INIT_DATA_MAX_SIZE,xSharedInitDataName, g_iPID);
		xInitDataLayout.Create(pSharedMemory);
	}

	std::string xInFileName("test1.txt");
	if (argc == 2)
	{
		xInFileName = argv[1];
	}
	std::fstream xInFileStream;
	xInFileStream.open(xInFileName, std::fstream::in);

	GlobalStrings xGlobalStrings;
	
	if (!xInFileStream.is_open())
	{
		//TODO: set defaults
		printf("Failed to load input file. Quitting. \n");
		return 1;
	}
	else
	{
		RM_ParseInput(xInFileStream, xInitDataLayout, xGlobalStrings);
	}

	//------------------------------------------------------------------------
	for (std::vector<std::string>::iterator it = xGlobalStrings.m_xMessageQueues.begin(); it != xGlobalStrings.m_xMessageQueues.end(); ++it)
	{
		std::string& xObjName = (*it);
		RM_MessageManager<RM_CHALLENGE_PROCESS_COUNT, RM_WToRMessageData> xMessageManager;
		xMessageManager.Create(g_iPID, xObjName);
	}

	for (std::vector<std::string>::iterator it = xGlobalStrings.m_xSharedBuffers.begin(); it != xGlobalStrings.m_xSharedBuffers.end(); ++it)
	{
		std::string& xObjName = (*it);
		std::string xLablesObjName(xObjName);
		xLablesObjName.append("Lables");

		RM_SharedMemory xSharedMemory;
		RM_SharedMemory xSharedLabelsMemory;
		xSharedMemory.Create(RM_ACCESS_READ | RM_ACCESS_WRITE, SHARED_MEMORY_MAX_SIZE, xObjName, g_iPID);
		xSharedLabelsMemory.Create(RM_ACCESS_READ | RM_ACCESS_WRITE, SHARED_MEMORY_LABESLS_MAX_SIZE, xLablesObjName, g_iPID);

		//initialise the shared memory by calling the constructors for the objects that map to the corresponding layout (the RM_SharedBuffer class)
		void* pSharedMemory = xSharedMemory.OpenMemory(RM_ACCESS_WRITE | RM_ACCESS_READ, SHARED_MEMORY_MAX_SIZE,xObjName, g_iPID);
		void* pLabelsMemory = xSharedLabelsMemory.OpenMemory(RM_ACCESS_WRITE | RM_ACCESS_READ, SHARED_MEMORY_LABESLS_MAX_SIZE,xLablesObjName, g_iPID);

		//initialise the buffers and call all the constructors (i.e. initialise locks, set counters to 0 etc)
		RM_SharedBuffer xSharedMemoryLayout;
		xSharedMemoryLayout.Create(pSharedMemory, pLabelsMemory);
	}

	RM_Job xJob;
	xJob.Initialise(RM_CHALLENGE_PROCESS_COUNT);

	RM_CustomProcess xProcesses[RM_CHALLENGE_PROCESS_COUNT];
	xProcesses[RM_CHALLENGE_PROCESS_WRITER].Initialise(L"Writer");
	xProcesses[RM_CHALLENGE_PROCESS_FIRSTREADER].Initialise(L"FirstReader");
	xProcesses[RM_CHALLENGE_PROCESS_SECONDREADER].Initialise(L"SecondReader");
	xProcesses[RM_CHALLENGE_PROCESS_WRITER_TRANSFORM].Initialise(L"WriterTransform");
	xProcesses[RM_CHALLENGE_PROCESS_READER_TRANSFORM].Initialise(L"ReaderTransform");
	for (u_int u = 0; u < RM_CHALLENGE_PROCESS_COUNT; ++u)
	{
		xJob.AddProcess(&xProcesses[u]);
	}

	printf("processes launched; press any key to kill all jobs \n");
	system("pause");

	xJob.Shutdown();

	return 0;
}