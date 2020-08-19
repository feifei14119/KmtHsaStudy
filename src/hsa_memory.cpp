#include "kmt.h"
#include "hsa.h"

void * HsaAllocCPU(uint64_t memSize)
{
	void * mem_addr = NULL;
	uint64_t mem_size = ALIGN_UP(memSize, gPageSize);

	mem_addr = KmtAllocHost(mem_size);
	KmtMapToGpu(mem_addr, mem_size);

	return mem_addr;
}
void * HsaAllocGPU(uint64_t memSize)
{
	void * mem_addr = NULL;
	uint64_t mem_size = ALIGN_UP(memSize, gPageSize);

	mem_addr = KmtAllocDevice(mem_size);
	KmtMapToGpu(mem_addr, mem_size);

	return mem_addr;
}

void HsaFreeMem(void * memAddr)
{
	KmtUnmapFromGpu(memAddr);
	KmtReleaseMemory(memAddr);
}
