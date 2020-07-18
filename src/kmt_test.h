#pragma once

#include <string>
#include <iostream>
#include <vector>

#include <stdio.h>
#include <strings.h>
#include <string.h> 
#include <iostream>
#include <vector>

#include <errno.h>
#include <assert.h>

#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/mman.h>

#include "../libhsakmt/linux/kfd_ioctl.h"

#include "../libhsakmt/hsakmttypes.h"
#include "../libhsakmt/hsakmt.h"

using namespace std;

#define KFD_DEVICE						"/dev/kfd"
#define DRM_RENDER_PATH					"/dev/dri"
#define DRM_RENDER_DEVICE(drm_minor)	std::string(std::string("/dev/dri/renderD") + std::to_string(drm_minor)).data()
#define KFD_GENERATION_ID				"/sys/devices/virtual/kfd/kfd/topology/generation_id"
#define KFD_SYSTEM_PROPERTIES			"/sys/devices/virtual/kfd/kfd/topology/system_properties"
#define PROC_CPUINFO_PATH				"/proc/cpuinfo"
#define KFD_NODES_PATH					"/sys/devices/virtual/kfd/kfd/topology/nodes/"
#define KFD_NODE(n)						std::string(std::string(KFD_NODES_PATH) + std::to_string(n)).data()

#define KFD_NODE_PROP(n)				std::string(std::string(KFD_NODE(n)) + "/properties").data()
#define KFD_NODE_MEM_PROP(n,m)			std::string(std::string(KFD_NODE(n)) + "/mem_banks/"+ std::to_string(m) + "/properties").data()
#define KFD_NODE_CACHE_PROP(n,m)		std::string(std::string(KFD_NODE(n)) + "/caches/"+ std::to_string(m) + "/properties").data()
#define KFD_NODE_LINK_PROP(n,m)			std::string(std::string(KFD_NODE(n)) + "/io_links/"+ std::to_string(m) + "/properties").data()

#define ALIGN_UP(x,align) (((uint64_t)(x) + (align) - 1) & ~(uint64_t)((align)-1))

extern int gKfdFd;
extern int gNodeNum;
extern int gNodeIdx;
extern int gGpuId;
extern int gPageSize;

// ==================================================================
// user API
// ==================================================================
extern void RunTest();
extern void MemTest();
extern void InitSdma();
extern void SdmaTest();
extern void SignalTest();
extern void QueueTest();
extern void CodeObjTest();

extern void Open();
extern void Close();
extern void GetInfo();

extern void MemInit();
extern void MemDeInit();
extern void * AllocMemoryAPU(uint64_t memSize);
extern void * AllocMemoryCPU(uint64_t memSize);
extern void * AllocMemoryGPUVM(uint64_t memSize, bool isHostAccess = false);
extern void * AllocMemoryMMIO();
extern void * AllocMemoryDoorbell(uint64_t memSize, uint64_t doorbell_offset);
extern void * AllocMemoryUserptr(void * memAddr, uint64_t memSize);
extern void FreeMemoryGPUVM(void * memAddr);
extern void FreeMemoryMMIO(void * memAddr);

extern void KmtCreateQueue(uint32_t queue_type, void * ring_buff, uint32_t ring_size, HsaQueueResource * queue_src);
extern HsaEvent * CreateSignal();


// ==================================================================
// inner functions
// ==================================================================
extern int kmtIoctl(int fd, unsigned long request, void *arg);
extern uint64_t kmtReadKey(std::string file, std::string key = "");


#if 0
// ==================================================================
// topology
// ==================================================================
typedef struct 
{
	uint32_t gpu_id;
	HsaNodeProperties node;
	HsaMemoryProperties *mem;     /* node->NumBanks elements */
	HsaCacheProperties *cache;
	HsaIoLinkProperties *link;
} node_props_t;
extern void kmt_topology_test();
#endif
