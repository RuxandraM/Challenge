#ifndef CHALLENGE_RM_EVENT
#define CHALLENGE_RM_EVENT

#include "Win10/RM_Platform_Event.h"

class RM_Event
{
public:
	void Create(int iIndex)
	{
		m_xPlatformEvent.Platform_Create(iIndex);
	}
	void Open(int iIndex)
	{
		m_xPlatformEvent.Platform_Open(iIndex);
	}

	void SendEvent()
	{
		m_xPlatformEvent.Platform_SendEvent();
	}
	void BlockingWait()
	{
		m_xPlatformEvent.Platform_BlcokingWait();
	}
private:
	RM_Platform_Event m_xPlatformEvent;
};

#endif//CHALLENGE_RM_EVENT
