#ifndef CHALLENGE_RM_FILE
#define CHALLENGE_RM_FILE

#include "Utils.h"
#include "RM_Event.h"
//TODO:real files
class RM_File
{
public:
	RM_File(const char* szFileName, u_int uNameLen) : m_szFileName(nullptr), m_pxIsReadyEvent(nullptr) , m_iIsOpen(0)
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
		printf("Opening File %s \n", m_szFileName);
		m_iIsOpen = 1;
		return RM_SUCCESS;
	}
	void Close()
	{
		printf("Closing File %s \n", m_szFileName);
		m_iIsOpen = 0;
	};

	bool IsOpen(){ return m_iIsOpen == 1; }

	void UpdateIsReadyEvent(RM_Event* pxEvent) { m_pxIsReadyEvent = pxEvent; }
	RM_Event* GetIsReadyEvent() { return m_pxIsReadyEvent; }
	void WriteData(void* pData, u_int uDataSize) 
	{
		char pGUGUTest[10];
		memcpy(pGUGUTest, pData, 9);
		pGUGUTest[9] = '\0';
		printf(" %s: %s \n", m_szFileName, pGUGUTest);
	}

private:
	//block copy ctor for now. I don't need to copy file classes, it's just to make sure I'm not sending by value
	RM_File(const RM_File&) {}
	RM_File& operator = (const RM_File&) { return *this; }

private:
	std::atomic<int> m_iIsOpen;
	char* m_szFileName;
	RM_Event* m_pxIsReadyEvent;
};

#endif//CHALLENGE_RM_FILE