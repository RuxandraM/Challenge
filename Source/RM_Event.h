#ifndef CHALLENGE_RM_EVENT
#define CHALLENGE_RM_EVENT

#include "Win10/RM_Platform_Event.h"

class RM_Event
{
public:
	void CreateNamedEvent(int iIndex, const char* szName)
	{
		m_xPlatformEvent.Platform_CreateNamedEvent(iIndex,szName);
	}
	void OpenNamedEvent(int iIndex, const char* szName)
	{
		m_xPlatformEvent.Platform_OpenNamedEvent(iIndex, szName);
	}
	
	void Create()
	{
		m_xPlatformEvent.Platform_Create();
	}

	void SetEvent()
	{
		m_xPlatformEvent.Platform_SetEvent();
	}

	bool BlockingWait()
	{
		return m_xPlatformEvent.Platform_BlcokingWait();
	}
private:
	RM_Platform_Event m_xPlatformEvent;
};

#endif//CHALLENGE_RM_EVENT
