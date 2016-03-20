#ifndef CHALLENGE_RM_PLATFORM_PROCESS
#define CHALLENGE_RM_PLATFORM_PROCESS

#include <windows.h>
#include <process.h>
#include <stdio.h>
#include "../Utils.h"



class Platform_CustomProcess
{
public:
	RM_RETURN_CODE Initialise(const WCHAR* szExecutable)
	{
		u_int uLen = static_cast<u_int>(wcslen(szExecutable));
		if (uLen >= 32) return RM_CUSTOM_ERR1;

		//add a process that writes data to the shared buffer
		STARTUPINFO xStartupInfo = {};

		xStartupInfo.cb = sizeof(STARTUPINFO);

		TCHAR lpszClientPath[32];
		memcpy(lpszClientPath, szExecutable, uLen * sizeof(WCHAR));
		lpszClientPath[uLen] = '\0';

		//DWORD xFlags = NORMAL_PRIORITY_CLASS | CREATE_UNICODE_ENVIRONMENT;
		//if (!xMainJobHandle)
		//{
		//	xFlags |= CREATE_NEW_PROCESS_GROUP;
		//}

		if (!CreateProcess(nullptr, lpszClientPath, nullptr, nullptr, FALSE,
			NORMAL_PRIORITY_CLASS | CREATE_NEW_CONSOLE | CREATE_UNICODE_ENVIRONMENT,
			//xFlags,
			nullptr, nullptr, &xStartupInfo, &m_xProcessInfo))
		{
			printf("Create Process %Ls Failed \n", szExecutable);
			return RM_CUSTOM_ERR2;
		}

		return RM_SUCCESS;
	}

	void Platform_Close() const
	{
		CloseHandle(m_xProcessInfo.hProcess);
	}

	RM_RETURN_CODE Attach(HANDLE xJobHandle) const
	{
		if (!AssignProcessToJobObject(xJobHandle, m_xProcessInfo.hProcess))
		{
			printf("AssignProcessToJobObject Failed \n");
			return RM_CUSTOM_ERR3;
		}
		return RM_SUCCESS;
	}

private:

	PROCESS_INFORMATION m_xProcessInfo;
};


class Platform_Job
{
public:
	void Platform_Initialise()
	{
		m_xMainJobHandle = CreateJobObject(nullptr, L"NanoporeChallengeJobObject");

		//tell the job to force killing all the processes when it is done
		JOBOBJECT_EXTENDED_LIMIT_INFORMATION xExtendedInfo = {};
		xExtendedInfo.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
		JOBOBJECTINFOCLASS xJobObjectInfoClass = {};
		SetInformationJobObject(m_xMainJobHandle, xJobObjectInfoClass, (void*)&xExtendedInfo,
			(u_int)sizeof(JOBOBJECT_EXTENDED_LIMIT_INFORMATION));
	}

	void Platform_Shutdown(u_int m_uNumAttachedProcesses)
	{
		UINT32 uExitCode = 0;
		BOOL bResult = TerminateJobObject(m_xMainJobHandle, uExitCode);

		CloseHandle(m_xMainJobHandle);
	}

	HANDLE Platform_GetHandle() const { return m_xMainJobHandle; }
	RM_RETURN_CODE Platform_AddProcess(const Platform_CustomProcess* pProcess)
	{
		return pProcess->Attach(m_xMainJobHandle);
	}


private:
	HANDLE m_xMainJobHandle;
};

#endif//CHALLENGE_RM_PLATFORM_PROCESS