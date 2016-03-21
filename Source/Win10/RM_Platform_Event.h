#ifndef CHALLENGE_RM_PLATFORM_EVENT
#define CHALLENGE_RM_PLATFORM_EVENT

#include "../Utils.h"
#include <windows.h>
#include <string>


class RM_Platform_Event
{
public:
	RM_Platform_Event():m_xEvent(nullptr){}
	~RM_Platform_Event()
	{
		Platform_Shutdown();
	}

	RM_RETURN_CODE Platform_CreateNamedEvent(int iIndex)
	{
		char buff[33];
		itostr(iIndex, buff);
		
		std::string xName(CHALLENGE_EVENT_NAME);
		xName.append(buff);

		std::wstring wName(xName.c_str(), xName.c_str() + strlen(xName.c_str()));

		HANDLE xResult = CreateEvent(nullptr, FALSE, FALSE, wName.c_str());
		if (xResult == nullptr)
		{
			printf("Main: Failed to create event (%d).\n", GetLastError());
			return RM_CUSTOM_ERR1;
		}
		return RM_SUCCESS;
	}

	void Platform_OpenNamedEvent(int iIndex)
	{
		char buff[33];
		itostr(iIndex, buff);

		std::string xName(CHALLENGE_EVENT_NAME);
		xName.append(buff);

		std::wstring wName(xName.c_str(), xName.c_str() + strlen(xName.c_str()));

		m_xEvent = OpenEvent(EVENT_ALL_ACCESS, FALSE, wName.c_str());
	}

	void Platform_Create()
	{
		//GUGU: manual reset to false?
		m_xEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	}

	void Platform_SetEvent()
	{
		if (!m_xEvent) return;
		SetEvent(m_xEvent);
	}

	bool Platform_BlcokingWait()
	{
		if (!m_xEvent) return false;
		WaitForSingleObject(m_xEvent, INFINITE);
		//GUGU!!! - do I need this?
		//ResetEvent(m_xEvent);
		return true;
	}

	void Platform_Shutdown()
	{
		if (m_xEvent)
		{
			CloseHandle(m_xEvent);
		}
	}

private:
	HANDLE m_xEvent;
};

#endif//CHALLENGE_RM_PLATFORM_EVENT
