#ifndef CHALLENGE_RM_THREAD
#define CHALLENGE_RM_THREAD

#include "Utils.h"
#include <process.h>
#include <windows.h>
#include "RM_Event.h"

class RM_Thread
{
public:
	virtual ~RM_Thread() {};
	
	HANDLE Start()
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

	virtual int Run(void* pParam) = 0;
	virtual void StartShutdown() {};

	static unsigned __stdcall ThreadFunc(void *pParam) 
	{
		if (pParam)
		{
			return ((RM_Thread*)pParam)->Run(pParam);
		}
		return 1;  // Return error
	}

protected:
	int m_id;

private:
	HANDLE m_hThread;
};

//A thread that is sleeping, waiting for events. It wakes up, deals with the event, and goes back to sleep.
class RM_EventThread : public RM_Thread
{
public:
	RM_EventThread() :m_bShouldTerminate(false) 
	{
		m_xEvent.Create();
	}
	virtual ~RM_EventThread() {}

	virtual void Execute(void *pParam) = 0;

	virtual int Run(void* pParam)
	{
		while (!m_bShouldTerminate)
		{
			if (!m_xEvent.BlockingWait())
			{
				//error with the thread events
				return 0;
			}
			Execute(pParam);
		}
		CleanUpBeforeTerminating();
		return 0;
	}

	RM_Event* GetEvent() { return &m_xEvent; }
	
	void StartShutdown()
	{
		m_bShouldTerminate = true;
		m_xEvent.SetEvent();
	}

protected:
	//called from the thread only, when it doesn't want to send events to itself
	void InternalStartShutdown(){ m_bShouldTerminate = true; }

private:
	//called just before exiting. Override this if the threads needs cleaning up
	virtual void CleanUpBeforeTerminating(){}

	std::atomic<bool> m_bShouldTerminate;
	//the event that the tread is always listening to
	RM_Event m_xEvent;
};

//small abstract class for a thread that signals a specific finish event when it's done.
//the finish event is unique per WorkerListenerThread.
class RM_WorkerListenerThread : public RM_EventThread
{
public:
	RM_WorkerListenerThread()
	{
		m_xFinishEvent.Create();
	}

	virtual void Execute(void *pParam) = 0;

	void CleanUpBeforeTerminating()
	{
		m_xFinishEvent.SetEvent();
	}

	RM_Event* GetFinishEvent() { return &m_xFinishEvent; }
private:
	RM_Event m_xFinishEvent;
};


#endif//CHALLENGE_RM_THREAD