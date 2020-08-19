/*
 * Copyright (C) 2014-2018 Advanced Micro Devices, Inc. All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#include <unistd.h>
#include <stdio.h>
#include <string>
#include "SDMAQueue.hpp"
#include "SDMAPacket.hpp"

#include "../src/kmt_test.h"

SDMAQueue::SDMAQueue(void) { CMD_NOP = 0; }

SDMAQueue::~SDMAQueue(void) {}

unsigned int SDMAQueue::Wptr()
{
    /* In SDMA queues write pointers are saved in bytes, convert the
     * wptr value to dword to fit the way BaseQueue works. On Vega10
     * the write ptr is 64-bit. We only read the low 32 bit (assuming
     * the queue buffer is smaller than 4GB) and modulo divide by the
     * queue size to simulate a 32-bit read pointer.
     */
    return (*m_Resources.Queue_write_ptr % m_QueueBuf->Size()) / sizeof(unsigned int);
}

unsigned int SDMAQueue::Rptr()
{
    /* In SDMA queues read pointers are saved in bytes, convert the
     * read value to dword to fit the way BaseQueue works. On Vega10
     * the read ptr is 64-bit. We only read the low 32 bit (assuming
     * the queue buffer is smaller than 4GB) and modulo divide by the
     * queue size to simulate a 32-bit read pointer.
     */
	printf("addr = 0x%016X\n", m_Resources.Queue_read_ptr);
	printf("size = %ld\n", m_QueueBuf->Size());
	printf("Rptr() = 0x%016X\n", (*m_Resources.Queue_read_ptr % m_QueueBuf->Size()) / sizeof(unsigned int));
    return (*m_Resources.Queue_read_ptr % m_QueueBuf->Size()) / sizeof(unsigned int);
}

unsigned int SDMAQueue::RptrWhenConsumed() 
{
    /* Rptr is same size and byte units as Wptr. Here we only care
     * about the low 32-bits. When all packets are consumed, read and
     * write pointers should have the same value.
     */
    return *m_Resources.Queue_write_ptr;
}

void SDMAQueue::SubmitPacket()
{
	// Vega10 and later uses 64-bit wptr and doorbell
	HSAuint64 wPtrInBytes = m_pendingWptr64 * sizeof(unsigned int);
	__sync_synchronize();//MemoryBarrier();
	*(m_Resources.Queue_write_ptr_aql) = wPtrInBytes;
	__sync_synchronize();//MemoryBarrier();
	*(m_Resources.Queue_DoorBell_aql) = wPtrInBytes;
}

void SDMAQueue::Wait4PacketConsumption(HsaEvent *event, unsigned int timeOut) 
{
      BaseQueue::Wait4PacketConsumption();
}


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
