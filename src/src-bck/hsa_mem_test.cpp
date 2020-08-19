#include <string>
#include <iostream>
#include <vector>
#include <unistd.h>
#include "hsa_test.h"

using namespace std;

int gMemNum; int gMemId = 0;
HsaMemoryProperties gTestMemProp;

enum AllocateEnum 
{
	AllocateNoFlags = 0,
	AllocateRestrict = (1 << 0),    // Don't map system memory to GPU agents
	AllocateExecutable = (1 << 1),  // Set executable permission
	AllocateDoubleMap = (1 << 2),   // Map twice VA allocation to backing store
	AllocateDirect = (1 << 3),      // Bypass fragment cache.
};

void test_sys_mem()
{
	printf("\n\t ======================\n");
	printf("\t system memory test.\n");

	HSAKMT_STATUS errNum;

	printf("\n\t\t ----------------------\n");
	printf("\t\t system memory info.\n");
	uint64_t physical_size = gTestMemProp.SizeInBytes;
	printf("\t\t physical size = %d(GB).\n", physical_size / 1024 / 1024 / 1024);

	uint64_t gpu_vm_size = (1ULL << 40);
	size_t page_size = 4096;
	uint64_t usr_mode_vm_size = 0x800000000000;// 64-bit linux
	uint64_t virtual_size = gpu_vm_size; // for apu: virtual_size = usr_mode_vm_size
	printf("\t\t page size = %d(B).\n", page_size);
	printf("\t\t os virtual size = %d(TB).\n", virtual_size / 1024 / 1024 / 1024 / 1024);
	uint64_t max_single_alloc_size = physical_size & ~(page_size - 1); // 向下对齐
	printf("\t\t max alloc size = AlignDown(physical_size, page_size) = %d(GB).\n", max_single_alloc_size / 1024 / 1024 / 1024);

	/* 
	 * host内存申请使用 hsa_memory_allocate() 函数
	 * --> <hsa.cpp> hsa_memory_allocate()
	 * --> <runtime.cpp> core::Runtime::runtime_singleton_->AllocateMemory() 
	 * --> <amd_memory_region.cpp> region->Allocate()
	 */
	printf("\n\t\t ----------------------\n");
	printf("\t\t system memory alloc test.\n");
	size_t want_alloc_size = 2 * 1024;
	size_t actual_alloc_size = 2 * 1024;
	actual_alloc_size = (want_alloc_size + page_size - 1) & ~(page_size - 1);// 向上对齐
	printf("\t\t user want alloc size = %d(B).\n", want_alloc_size);
	printf("\t\t actual alloc size = AlignUp(want_alloc_size, page_size) = %d(B).\n", actual_alloc_size);

	uint32_t alloc_flag = 0;

	HsaMemFlags mem_flag;
	mem_flag.Value = 0;
	mem_flag.ui32.PageSize = HSA_PAGE_SIZE_4KB;
	mem_flag.ui32.NoSubstitute = 1;
	mem_flag.ui32.HostAccess = 1;
	mem_flag.ui32.CachePolicy = HSA_CACHING_CACHED;
	mem_flag.ui32.CoarseGrain = 0;// (fine_grain) ? 0 : 1;
	mem_flag.ui32.ExecuteAccess = (alloc_flag & AllocateExecutable ? 1 : 0);
	mem_flag.ui32.AQLQueueMemory = (alloc_flag & AllocateDoubleMap ? 1 : 0);

	HsaMemMapFlags map_flag;
	map_flag.Value = 0;
	map_flag.ui32.HostAccess = 1;
	map_flag.ui32.PageSize = HSA_PAGE_SIZE_4KB;

	void * alloc_addr = NULL; 
	errNum = hsaKmtAllocMemory(gNodeId, actual_alloc_size, mem_flag, &alloc_addr);

	if (errNum != HSAKMT_STATUS_SUCCESS)
		printf("\t\t faild to alloc sys mem. address = null.\n");
	if(alloc_addr == NULL)
		printf("\t\t faild to alloc sys mem. address = null.\n");

	printf("\t\t alloc sys mem %d(B) at 0x%X.\n", actual_alloc_size, alloc_addr);

	void * kmt_alloc_addr = NULL;
//	ff_kmt_alloc_host_cpu(&kmt_alloc_addr, actual_alloc_size, mem_flag);

	printf("\n\t\t ----------------------\n");
	printf("\t\t map to gpu node test.\n");
	size_t map_node_count = 1;
	uint32_t* map_node_id;
	if ((alloc_flag & AllocateRestrict) == 0) 
	{
		// Map to all GPU agents.
		map_node_count = gGpuNodeNum;
		map_node_id = &gGpuNodeIds[0];
	}

	uint64_t alternate_va = 0;
	errNum = hsaKmtMapMemoryToGPUNodes(alloc_addr, actual_alloc_size, &alternate_va, map_flag, map_node_count, map_node_id);
	if (errNum == HSAKMT_STATUS_SUCCESS)
		printf("\t\t map %d(B) sys mem at 0x%X to %d gpu node.\n", actual_alloc_size, alloc_addr, gGpuNodeNum);
	else
		printf("\t\t faild to map sys mem to gpu node. err = %d\n", errNum);

	printf("\n\t\t ----------------------\n");
	printf("\t\t unmap to gpu node test.\n");
	errNum = hsaKmtUnmapMemoryToGPU(alloc_addr);
	if (errNum == HSAKMT_STATUS_SUCCESS)
		printf("\t\t unmap sys mem at 0x%X to gpu node.\n", alloc_addr);
	else
		printf("\t\t faild to unmap sys mem to gpu node. err = %d\n", errNum);

	printf("\n\t\t ----------------------\n");
	printf("\t\t free system memory test.\n");
	errNum = hsaKmtFreeMemory(alloc_addr, actual_alloc_size);
	if (errNum == HSAKMT_STATUS_SUCCESS)
		printf("\t\t free sys mem %d(B) at 0x%X.\n", actual_alloc_size, alloc_addr);
	else
		printf("\t\t faild to free sys mem. err = %d\n", errNum);

//	ff_kmt_free_host_cpu(kmt_alloc_addr, actual_alloc_size);

	printf("\n");
}

void test_gpu_mem()
{
	printf("\n\t ======================\n");
	printf("\t gpu public/private memory test.\n");

	HSAKMT_STATUS errNum;

	printf("\n\t\t ----------------------\n");
	printf("\t\t gpu memory info.\n");
	uint64_t physical_size = gTestMemProp.SizeInBytes;
	printf("\t\t physical size = %.3f(GB).\n", physical_size / 1024.0 / 1024 / 1024);

	uint64_t gpu_vm_size = (1ULL << 40);
	size_t page_size = 4096;
	uint64_t usr_mode_vm_size = 0x800000000000;// 64-bit linux
	uint64_t virtual_size = gpu_vm_size; // for apu: virtual_size = usr_mode_vm_size
	printf("\t\t page size = %d(B).\n", page_size);
	printf("\t\t os virtual size = %d(TB).\n", virtual_size / 1024 / 1024 / 1024 / 1024);
	uint64_t max_single_alloc_size = physical_size & ~(page_size - 1); // 向下对齐
	printf("\t\t max alloc size = AlignDown(physical_size, page_size) = %d(GB).\n", max_single_alloc_size / 1024 / 1024 / 1024);

	/*
	 * host内存申请使用 hsa_memory_allocate() 函数
	 * --> <hsa.cpp> hsa_memory_allocate()
	 * --> <runtime.cpp> core::Runtime::runtime_singleton_->AllocateMemory()
	 * --> <amd_memory_region.cpp> region->Allocate()
	 */
	printf("\n\t\t ----------------------\n");
	printf("\t\t gpu public/private memory alloc test.\n");
	size_t want_alloc_size = 2 * 1024;
	size_t actual_alloc_size = 2 * 1024;
	actual_alloc_size = (want_alloc_size + page_size - 1) & ~(page_size - 1);// 向上对齐
	printf("\t\t user want alloc size = %d(B).\n", want_alloc_size);
	printf("\t\t actual alloc size = AlignUp(want_alloc_size, page_size) = %d(B).\n", actual_alloc_size);
	
	uint32_t alloc_flag = 0;

	HsaMemFlags mem_flag;
	mem_flag.Value = 0;
	mem_flag.ui32.PageSize = HSA_PAGE_SIZE_4KB;
	mem_flag.ui32.NoSubstitute = 1;
	mem_flag.ui32.HostAccess = (gTestMemProp.HeapType == HSA_HEAPTYPE_FRAME_BUFFER_PRIVATE) ? 0 : 1;
	mem_flag.ui32.NonPaged = 1;
	mem_flag.ui32.CoarseGrain = 0;// (fine_grain) ? 0 : 1;
	mem_flag.ui32.ExecuteAccess = (alloc_flag & AllocateExecutable ? 1 : 0);
	mem_flag.ui32.AQLQueueMemory = (alloc_flag & AllocateDoubleMap ? 1 : 0);

	HsaMemMapFlags map_flag;
	map_flag.Value = 0;
	map_flag.ui32.PageSize = HSA_PAGE_SIZE_4KB;

	void * alloc_addr = NULL;
	errNum = hsaKmtAllocMemory(gNodeId, actual_alloc_size, mem_flag, &alloc_addr);

	if (errNum != HSAKMT_STATUS_SUCCESS)
		printf("\t\t faild to alloc gpu mem. address = null.\n");
	if (alloc_addr == NULL)
		printf("\t\t faild to alloc gpu mem. address = null.\n");

	printf("\t\t alloc gpu mem %d(B) at 0x%X.\n", actual_alloc_size, alloc_addr);

	printf("\n\t\t ----------------------\n");
	printf("\t\t map to gpu node test.\n");
	size_t map_node_count = 1;
	uint32_t* map_node_id = (uint32_t*)&gNodeId;

	uint64_t alternate_va = 0;
	errNum = hsaKmtMapMemoryToGPUNodes(alloc_addr, actual_alloc_size, &alternate_va, map_flag, map_node_count, map_node_id);
	if (errNum == HSAKMT_STATUS_SUCCESS)
		printf("\t\t map %d(B) gpu mem at 0x%X to gpu node %d.\n", actual_alloc_size, alloc_addr, *map_node_id);
	else
		printf("\t\t faild to map gpu mem to gpu node. err = %d\n", errNum);

	printf("\n\t\t ----------------------\n");
	printf("\t\t unmap to gpu node test.\n");
	errNum = hsaKmtUnmapMemoryToGPU(alloc_addr);
	if (errNum == HSAKMT_STATUS_SUCCESS)
		printf("\t\t unmap gpu mem at 0x%X to gpu node.\n", alloc_addr);
	else
		printf("\t\t faild to unmap gpu mem to gpu node. err = %d\n", errNum);

	printf("\n\t\t ----------------------\n");
	printf("\t\t free gpu public/private memory test.\n");
	errNum = hsaKmtFreeMemory(alloc_addr, actual_alloc_size);
	if (errNum == HSAKMT_STATUS_SUCCESS)
		printf("\t\t free gpu mem %d(B) at 0x%X.\n", actual_alloc_size, alloc_addr);
	else
		printf("\t\t faild to free gpu mem. err = %d\n", errNum);

	printf("\n");
}

void hsa_mem_test()
{
	printf("***********************\n");
	printf("* hsa memory test     *\n");
	printf("***********************\n");

	vector<HsaMemoryProperties> mem_props(gMemNum);
	hsaKmtGetNodeMemoryProperties(gNodeId, gMemNum, &mem_props[0]);
	printf("node %d has %d mem, chose %d mem\n", gNodeId, gMemNum, gMemId);
	gTestMemProp = mem_props[gMemId];
	
	switch (gTestMemProp.HeapType)
	{
	case HSA_HEAPTYPE_SYSTEM:
		test_sys_mem();
		break;

	case HSA_HEAPTYPE_FRAME_BUFFER_PUBLIC:
	case HSA_HEAPTYPE_FRAME_BUFFER_PRIVATE:
		test_gpu_mem();
		break;
	}

	printf("sys page size = %ld.\n", sysconf(_SC_PAGESIZE));
}

/*
HSA
HSA_HEAPTYPE						HSA_REGION_INFO_SEGMENT			HSA_REGION_INFO_GLOBAL_FLAGS

HSA_HEAPTYPE_SYSTEM					HSA_REGION_SEGMENT_GLOBAL		HSA_REGION_GLOBAL_FLAG_KERNARG|HSA_REGION_GLOBAL_FLAG_FINE_GRAINED	system_region kernarg_region
HSA_HEAPTYPE_FRAME_BUFFER_PUBLIC	HSA_REGION_SEGMENT_GLOBAL		HSA_REGION_GLOBAL_FLAG_COARSE_GRAINED								gpu_local_region
HSA_HEAPTYPE_FRAME_BUFFER_PRIVATE	HSA_REGION_SEGMENT_GLOBAL		HSA_REGION_GLOBAL_FLAG_COARSE_GRAINED								gpu_local_region
HSA_HEAPTYPE_GPU_GDS
HSA_HEAPTYPE_GPU_LDS				HSA_REGION_SEGMENT_GROUP
HSA_HEAPTYPE_GPU_SCRATCH
HSA_HEAPTYPE_DEVICE_SVM

									HSA_REGION_SEGMENT_READONLY
									HSA_REGION_SEGMENT_PRIVATE
									HSA_REGION_SEGMENT_KERNARG


full_profile = is_apu_node (false)
fine_grain   = true 
*/

