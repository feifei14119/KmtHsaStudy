#pragma once

#include <string>
#include <iostream>
#include <vector>

#include <stdio.h>
#include <strings.h>
#include <string.h> 
#include <cstring>
#include <cstdlib>
#include <iostream>
#include <vector>
#include <fstream>
#include <sstream>

#include <errno.h>
#include <assert.h>

#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/mman.h>

#include "../include/linux/kfd_ioctl.h"
#include "../include/hsakmttypes.h"

using namespace std;

// ==================================================================
// kmt inner functions
// ==================================================================
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
// KMT API
// ==================================================================
extern int kmtIoctl(int fd, unsigned long request, void *arg);
extern uint64_t kmtReadKey(std::string file, std::string key = "");

extern void kmtInitMem();
extern void kmtDeInitMem();

// ------------------------------------------------------------------
extern void * KmtAllocDoorbell(uint64_t memSize, uint64_t doorbell_offset);
extern void * KmtAllocDevice(uint64_t memSize);
extern void * KmtAllocHost(uint64_t memSize, bool isPage = true);
extern void KmtReleaseMemory(void * memAddr);
extern void KmtMapToGpu(void * memAddr, uint64_t memSize, uint64_t * gpuvm_address = NULL);
extern void KmtUnmapFromGpu(void * memAddr);
extern void * KmtGetVmHandle(void * memAddr);

extern void KmtCreateQueue(uint32_t queue_type, void * ring_buff, uint32_t ring_size, HsaQueueResource * queue_src);
extern void KmtDestroyQueue(uint64_t QueueId);

extern HsaEvent * KmtCreateEvent();
extern void KmtDestroyEvent(HsaEvent *Event);
extern void KmtSetEvent(HsaEvent *Event);
extern void KmtWaitOnEvent(HsaEvent *Event);

// ==================================================================
// HSA API
// ==================================================================
extern void hsaInitSdma();
extern void hsaDeInitSdma();

// ------------------------------------------------------------------
extern void KmtHsaInit();
extern void KmtHsaDeInit();

extern void * HsaAllocCPU(uint64_t memSize);
extern void * HsaAllocGPU(uint64_t memSize);
extern void HsaFreeMem(void * memAddr);

extern void HsaSdmaWrite(void * dst, uint32_t data);
extern void HsaSdmaCopy(void * dst, void * src, uint32_t copy_size);

extern void * HsaLoadKernel(string fileName);

extern void HsaAqlCreate();
extern void HsaAqlSetPkt(void * kernelHandle, uint32_t grpSz = 1, uint32_t grdSz = 1);
extern void HsaAqlSetKernelArg(void ** arg, size_t argSize);
extern void HsaAqlRingDoorbell();

// ==================================================================
// TEMP TEST
// ==================================================================
extern void RunKmtTest();

extern void RunMemoryTest();
extern void RunSdmaTest();
extern void RunEventTest();
extern void RunAqlTest();

