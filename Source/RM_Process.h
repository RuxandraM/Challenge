#ifndef CHALLENGE_RM_PROCESS
#define CHALLENGE_RM_PROCESS

#include "Win10/Platform_Process.h"

class RM_CustomProcess
{
public:
	RM_RETURN_CODE Initialise(const WCHAR* szExecutable)
	{
		return m_xCustomProcess.Initialise(szExecutable);
	}

	void Close() const
	{
		m_xCustomProcess.Platform_Close();
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