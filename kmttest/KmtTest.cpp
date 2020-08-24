#include <unistd.h>
#include <stdio.h>
#include <string>
#include "SDMAQueue.hpp"
#include "SDMAPacket.hpp"

#include "../include/hsakmt.h"
#include "../src/kmthsa.h"

#if 0
extern HsaMemFlags cpu_mem_flag;
extern HsaMemFlags gpu_mem_flag;
void SubmitPacketSdmaQueue() 
{
	HSAKMT_STATUS status;
	status = hsaKmtOpenKFD();
	HsaSystemProperties SystemProperties;
	hsaKmtAcquireSystemProperties(&SystemProperties);

	HsaNodeProperties NodeProperties[2];
	HsaMemoryProperties * MemoryProperties[2];

	hsaKmtGetNodeProperties(0, &NodeProperties[0]); 
	MemoryProperties[0] = (HsaMemoryProperties*)malloc(NodeProperties[0].NumMemoryBanks * sizeof(HsaMemoryProperties));
	status = hsaKmtGetNodeMemoryProperties(0, NodeProperties[0].NumMemoryBanks, MemoryProperties[0]);

	hsaKmtGetNodeProperties(1, &NodeProperties[1]);
	MemoryProperties[1] = (HsaMemoryProperties*)malloc(NodeProperties[1].NumMemoryBanks * sizeof(HsaMemoryProperties));
	status = hsaKmtGetNodeMemoryProperties(1, NodeProperties[1].NumMemoryBanks, MemoryProperties[1]);

	/////////////////////

	float * h_A;
	int defaultGPUNode = 1;
	Open();
	GetInfo();

	cpu_mem_flag.Value = 0;
	cpu_mem_flag.ui32.NoNUMABind = 1;
	cpu_mem_flag.ui32.ExecuteAccess = 1;
	cpu_mem_flag.ui32.HostAccess = 1;
	cpu_mem_flag.ui32.NonPaged = 0;
	cpu_mem_flag.ui32.CoarseGrain = 0;
	cpu_mem_flag.ui32.PageSize = HSA_PAGE_SIZE_4KB;

	gpu_mem_flag.Value = 0;
	gpu_mem_flag.ui32.NoSubstitute = 1;
	gpu_mem_flag.ui32.NonPaged = 1;
	gpu_mem_flag.ui32.CoarseGrain = 1;
	gpu_mem_flag.ui32.PageSize = HSA_PAGE_SIZE_4KB;

	MemInit();
	printf("=============================\n");
	//HsaMemoryBuffer destBuf(4096, 0, false); // CPU 
	h_A = (float*)AllocMemoryCPU(1024 * sizeof(float));
	printf("=============================\n");

	//////////////////////////
	SDMAQueue queue;

	queue.Create(defaultGPUNode);
	queue.PrintResrouces();
	   	 
	SDMAWriteDataPacket pkt(5, h_A, 0x02020202);
	pkt.Dump();
	queue.PlaceAndSubmitPacket(pkt);
	queue.PrintResrouces();
	queue.Wait4PacketConsumption();
	queue.PrintResrouces();

	uint32_t value = *(uint32_t *)h_A;
	printf("read back value = 0x%08X\n", value);
	int rtn = WaitOnValue((const unsigned int *)h_A, 0x02020202);
	printf("wait return = %d \n", rtn);
	   
	queue.Destroy();

	//TEST_END
	hsaKmtCloseKFD();
}
void SdmaConcurrentCopies()
{
	HSAKMT_STATUS status;
	status = hsaKmtOpenKFD();
	HsaSystemProperties SystemProperties;
	hsaKmtAcquireSystemProperties(&SystemProperties);

	HsaNodeProperties NodeProperties[2];
	HsaMemoryProperties * MemoryProperties[2];

	hsaKmtGetNodeProperties(0, &NodeProperties[0]);
	MemoryProperties[0] = (HsaMemoryProperties*)malloc(NodeProperties[0].NumMemoryBanks * sizeof(HsaMemoryProperties));
	status = hsaKmtGetNodeMemoryProperties(0, NodeProperties[0].NumMemoryBanks, MemoryProperties[0]);

	hsaKmtGetNodeProperties(1, &NodeProperties[1]);
	MemoryProperties[1] = (HsaMemoryProperties*)malloc(NodeProperties[1].NumMemoryBanks * sizeof(HsaMemoryProperties));
	status = hsaKmtGetNodeMemoryProperties(1, NodeProperties[1].NumMemoryBanks, MemoryProperties[1]);

	/////////////////////

	Open();
	GetInfo();
	float * h_A, *h_B, *d_C;
	uint32_t len = 1024;
	int defaultGPUNode = 1;

	cpu_mem_flag.Value = 0;
	cpu_mem_flag.ui32.NoNUMABind = 1;
	cpu_mem_flag.ui32.ExecuteAccess = 1;
	cpu_mem_flag.ui32.HostAccess = 1;
	cpu_mem_flag.ui32.NonPaged = 0;
	cpu_mem_flag.ui32.CoarseGrain = 0;
	cpu_mem_flag.ui32.PageSize = HSA_PAGE_SIZE_4KB;

	gpu_mem_flag.Value = 0;
	gpu_mem_flag.ui32.NoSubstitute = 1;
	gpu_mem_flag.ui32.NonPaged = 1;
	gpu_mem_flag.ui32.CoarseGrain = 1;
	gpu_mem_flag.ui32.PageSize = HSA_PAGE_SIZE_4KB;

	MemInit();
	printf("=============================\n");
	//HsaMemoryBuffer srcBuf1(len * sizeof(float), 0, false);
	//HsaMemoryBuffer srcBuf2(len * sizeof(float), 0, false);
	//HsaMemoryBuffer destBuf(len * sizeof(float), defaultGPUNode, false);
	h_A = (float*)AllocMemoryCPU(len * sizeof(float));
	h_B = (float*)AllocMemoryCPU(len * sizeof(float));
	d_C = (float*)AllocMemoryGPUVM(len * sizeof(float));
	printf("=============================\n");

	for (uint32_t i = 0; i < len; i++)
		h_A[i] = i * 1.0f;
	for (uint32_t i = 0; i < len; i++)
		h_B[i] = i * -1.0f;

	//////////////////////////
	SDMAQueue queue;
	queue.Create(defaultGPUNode);
	queue.PrintResrouces();

	SDMACopyDataPacket pkt(5, d_C, h_A, len * sizeof(float));
	pkt.Dump();
	queue.PlaceAndSubmitPacket(pkt);
	queue.PrintResrouces();
	queue.Wait4PacketConsumption();
	queue.PrintResrouces();

	SDMACopyDataPacket pkt2(5, h_B, d_C, len * sizeof(float));
	pkt2.Dump();
	queue.PlaceAndSubmitPacket(pkt2);
	queue.PrintResrouces();
	queue.Wait4PacketConsumption();
	queue.PrintResrouces();

	printf("=============================\n");
	for (uint32_t i = 0; i < 16; i++)
	{
		printf("%.1f, ", h_A[i]);
		if ((i + 1) % 8 == 0)
			printf("\n");
	}
	printf("... ...\n");
	for (uint32_t i = len - 16; i < len; i++)
	{
		printf("%.1f, ", h_A[i]);
		if ((i + 1) % 8 == 0)
			printf("\n");
	}
	printf("-----------------------------\n");
	for (uint32_t i = 0; i < 16; i++)
	{
		printf("%.1f, ", h_B[i]);
		if ((i + 1) % 8 == 0)
			printf("\n");
	}
	printf("... ...\n");
	for (uint32_t i = len - 16; i < len; i++)
	{
		printf("%.1f, ", h_B[i]);
		if ((i + 1) % 8 == 0)
			printf("\n");
	}
	printf("=============================\n");
	
	queue.Destroy();

	//TEST_END
	hsaKmtCloseKFD();
}
#endif
HsaEvent * m_pHsaEvent;
void CreateDestroyEvent()
{
	HsaEventDescriptor Descriptor;

	Descriptor.EventType = HSA_EVENTTYPE_SIGNAL;
	Descriptor.SyncVar.SyncVar.UserData = (void*)0xABCDABCD;
	Descriptor.NodeId = 1;

	hsaKmtCreateEvent(&Descriptor, false, false, &m_pHsaEvent);
}

void SignalEvent()
{
	//PM4Queue queue;
	HsaEvent *tmp_event;

	int defaultGPUNode = 1;

	CreateQueueTypeEvent(false, false, defaultGPUNode, &tmp_event);
	CreateQueueTypeEvent(false, false, defaultGPUNode, &m_pHsaEvent);
	//ASSERT_NE(0, m_pHsaEvent->EventData.HWData2);

	//queue.Create(defaultGPUNode);
	//queue.PlaceAndSubmitPacket(PM4ReleaseMemoryPacket(m_FamilyId, false, m_pHsaEvent->EventData.HWData2, m_pHsaEvent->EventId));
	//queue.Wait4PacketConsumption();

	hsaKmtWaitOnEvent(m_pHsaEvent, 2000);
	hsaKmtDestroyEvent(tmp_event);
	//queue.Destroy();
}

void RunKmtTest()
{
	CreateDestroyEvent();
	//SignalEvent();
}
