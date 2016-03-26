#ifndef CHALLENGE_RM_FILE
#define CHALLENGE_RM_FILE

#include "Utils.h"
#include "RM_Event.h"
#include "RM_Mutex.h"

//TODO:real HDF5 files
#include <fstream>


class RM_File
{
public:
	RM_File(const char* szFileName, u_int uNameLen, RM_ACCESS_FLAG eAccessFlag) : 
		m_szFileName(nullptr), m_pxIsReadyEvent(nullptr) , m_iIsOpen(0)
	{
		m_szFileName = new char[uNameLen+1];
		memcpy(m_szFileName, szFileName, uNameLen);
		m_szFileName[uNameLen] = '\0';
	}
	~RM_File()
	{
		delete [] m_szFileName;
	}

	RM_RETURN_CODE Open(bool bCreateIfNotExisting) 
	{ 
		m_xFileMutex.Lock();
		printf("Opening File %s \n", m_szFileName);
		m_xFile.open(m_szFileName, std::fstream::out);
		//TODO:
		//file = H5Fcreate(FILE, H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT);

		const u_int uDealyInNanoS = rand() % CHALLENGE_OPEN_FILE_DELAY_IN_NANOSEC;
		Sleep(uDealyInNanoS);

		m_iIsOpen = 1;
		m_xFileMutex.Unlock();
		return RM_SUCCESS;
	}

	void Close()
	{
		m_xFileMutex.Lock();
		if (m_iIsOpen != 0)
		{
			printf("Closing File %s \n", m_szFileName);
			m_xFile.close();
			m_iIsOpen = 0;
		}
		m_xFileMutex.Unlock();
	};

	bool IsOpen(){ return m_iIsOpen == 1; }

	void UpdateIsReadyEvent(RM_Event* pxEvent) { m_pxIsReadyEvent = pxEvent; }
	RM_Event* GetIsReadyEvent() { return m_pxIsReadyEvent; }
	void WriteData(void* pData, u_int uDataSize) 
	{
		if (m_iIsOpen == 0)
		{
			printf("Attempt to write to file when it is not open! \n");
			return;
		}

		//deliberately slow down here to pretend we are writing to a big file
		const u_int uDealyInNanoS = rand() % CHALLENGE_WRITE_FILE_DELAY_IN_NANOSEC;
		Sleep(uDealyInNanoS);

		char szBuffer[10];
		memcpy(szBuffer, pData, 9);
		szBuffer[9] = '\0';
		
		#ifdef DEBUG_OUTPUT_TO_CONSOLE
			printf(" %s: %s \n", m_szFileName, szBuffer);
		#endif//DEBUG_OUTPUT_TO_CONSOLE

		//TODO: I'm not outputting everything here
		m_xFile << szBuffer <<"\n";
	}

	const char* GetFileName() const
	{
		return m_szFileName;
	}

private:
	//block copy ctor for now. I don't need to copy file classes, it's just to make sure I'm not sending by value
	RM_File(const RM_File&) {}
	RM_File& operator = (const RM_File&) { return *this; }

private:
	std::atomic<int> m_iIsOpen;
	RM_PlatformMutex m_xFileMutex;
	char* m_szFileName;
	u_int m_uAccessFlags;	//combination of RM_ACCESS_FLAG
	RM_Event* m_pxIsReadyEvent;

	std::fstream m_xFile;
};

#endif//CHALLENGE_RM_FILE