#pragma once

#include <vector>
#include <hsa.h>
#include <hsa_ext_amd.h>

#include "ff_basic.h"
#include "ff_hsa_helper.h"
#include "ff_file_opt.h"
#include "ff_log.h"
#include "ff_cmd_args.h"

namespace feifei 
{
	/************************************************************************/
	/* 硬件信息																*/
	/************************************************************************/
	//#define ISA_GFX800			(1)
#define	ISA_GFX900			(1)
#define	SE_NUM				(4)
#define	CU_PER_SE			(16)
#define CU_NUM				(CU_PER_SE * SE_NUM)
#define	WAVE_SIZE			(64)
#define SIMD_PER_CU			(64)
#define CACHE_LINE			(16)
#define	MAX_VGPR_COUNT		(256)
#define	MAX_SGPR_COUNT		(800)
#define MAX_LDS_SIZE		(64*1024)

	class AgentHSA
	{
	public:
		AgentHSA(hsa_agent_t agent)
		{
			this->agent = agent;
			getAgentInfo();
		}
		void PrintAgentInfoFull();
		void PrintAgentInfoShort();

	protected:
		hsa_agent_t agent;
		T_HsaAgentInfo agentInfo;

		void getAgentInfo();
	};

	class RuntimeHSA
	{
	private:
		static RuntimeHSA * pInstance;
		RuntimeHSA()
		{
		}

	protected:
		T_HsaSystemInfo hsaSystemInfo;
		std::vector<AgentHSA*> agents;
		AgentHSA * selAgent;

		E_ReturnState initPlatform();
		void getHsaSystemInfo();

		hsa_queue_t* queue;
		uint32_t queue_size;

	public:
		~RuntimeHSA()
		{
			hsa_shut_down();
		}
	
		static RuntimeHSA * GetInstance();
		T_HsaSystemInfo *HsaSystemInfo() { return &hsaSystemInfo; }
		void PrintRuntimeInfo(bool isFullInfo = false);
		void PrintHsaSystemInfo();

		uint DevicesCnt() { return agents.size(); }
		std::vector<AgentHSA*> *Devices() { return &agents; }
		E_ReturnState SellectDevice(uint devNum) { selAgent = agents[devNum]; }
	};
}
