#ifndef CHALLENGE_RM_PROCESS
#define CHALLENGE_RM_PROCESS

#include "Win10/Platform_Process.h"

class RM_CustomProcess
{
public:
	RM_RETURN_CODE Initialise(const WCHAR* szExecutable)
	{
		return m_xCustomProcess.Initialise(szExecutable);

		/*
		u_int uLen = static_cast<u_int>(wcslen(szExecutable));
		if (uLen >= 32) return S_FALSE;

		//add a process that writes data to the shared buffer
		STARTUPINFO xStartupInfo = {};

		xStartupInfo.cb = sizeof(STARTUPINFO);

		TCHAR lpszClientPath[32];
		memcpy(lpszClientPath, szExecutable, uLen * sizeof(WCHAR));
		lpszClientPath[uLen] = '\0';

		DWORD xFlags = NORMAL_PRIORITY_CLASS | CREATE_UNICODE_ENVIRONMENT;
		if (!xMainJobHandle)
		{
		xFlags |= CREATE_NEW_PROCESS_GROUP;
		}

		if (!CreateProcess(nullptr, lpszClientPath, nullptr, nullptr, FALSE,
		NORMAL_PRIORITY_CLASS | CREATE_NEW_CONSOLE | CREATE_UNICODE_ENVIRONMENT,
		//xFlags,
		nullptr, nullptr, &xStartupInfo, &m_xProcessInfo))
		{
		printf("Create Process %Ls Failed \n", szExecutable);
		return S_FALSE;
		}

		if (!xMainJobHandle)
		{
		return S_OK;
		}

		if (!AssignProcessToJobObject(xMainJobHandle, m_xProcessInfo.hProcess))
		{
		printf("AssignProcessToJobObject Failed \n");
		return S_FALSE;
		}

		return S_OK;
		*/
	}

	void Close() const
	{
		m_xCustomProcess.Platform_Close();
		//CloseHandle(m_xProcessInfo.hProcess);
	}

	const Platform_CustomProcess* GetPlatformProcess() const { return &m_xCustomProcess; }

private:

	Platform_CustomProcess m_xCustomProcess;
};

class RM_Job
{
public:
	RM_Job() :m_uNumAttachedProcesses(0) {}

	void Initialise(u_int uMaxNumProcesses)
	{
		m_uMaxNumProcesses = uMaxNumProcesses;
		m_ppProcesses = new const RM_CustomProcess*[uMaxNumProcesses];
		m_xJob.Platform_Initialise();
	}

	void Shutdown()
	{
		if (m_ppProcesses)
		{
			for (u_int u = 0; u < m_uNumAttachedProcesses; ++u)
			{
				if (m_ppProcesses[u])
				{
					m_ppProcesses[u]->Close();
				}
			}
		}
		delete[] m_ppProcesses;
		m_xJob.Platform_Shutdown(m_uNumAttachedProcesses);
	}

	void AddProcess(const RM_CustomProcess* pxProcess)
	{
		if (!m_ppProcesses) return;
		m_ppProcesses[m_uNumAttachedProcesses++] = pxProcess;
		m_xJob.Platform_AddProcess(pxProcess->GetPlatformProcess());
	}

private:
	u_int m_uMaxNumProcesses;
	u_int m_uNumAttachedProcesses;
	const RM_CustomProcess** m_ppProcesses;
	Platform_Job m_xJob;
};



#endif//CHALLENGE_RM_PROCESS