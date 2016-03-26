#ifndef CHALLENGE_RM_SHARED_Init_DATA
#define CHALLENGE_RM_SHARED_Init_DATA

#include "Utils.h"
#include <sstream>

class RM_SharedInitDataLayout
{
public:
	
	void MapMemory(void* pMemory)
	{	
		m_pxData = reinterpret_cast<RM_InitData*>(pMemory);
	}

	void Create(void* pMemory)
	{
		MapMemory(pMemory);
		new (pMemory) RM_InitData;
	}

	const RM_InitData& GetInitData(int iProcessIndex)
	{
		return m_pxData[iProcessIndex];
	}

	RM_InitData* GetInitDataPtr(int iProcessIndex) { return &m_pxData[iProcessIndex]; }
	
private:
	RM_InitData* m_pxData;
};

#endif//CHALLENGE_RM_SHARED_Init_DATA