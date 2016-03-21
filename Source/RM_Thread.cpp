
#include "RM_Thread.h"

//RM_Thread::RM_Thread(HANDLE eventToWait) : m_xEventHandle(eventToWait), 
//m_bTerminate(false)
//{}
/*
HANDLE RM_Thread::Start() 
{
	unsigned threadId = 0;
	m_hThread = (HANDLE)_beginthreadex(
		NULL,		// no security attributes (child cannot inherited handle)
		1024 * 1024,	// 1MB stack size
		ThreadFunc,	// code to run on new thread
		this,		// pointer to host application class
		0,			// run immediately (could create suspended)
		&threadId	// OUT: returns thread ID
		);
	return m_hThread;
}
*/