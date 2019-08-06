#pragma once

#include <string>
#include <iostream>
#include <vector>

#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <strings.h>
#include <string.h> 
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>

#include "../libhsakmt/hsakmttypes.h"
#include "../libhsakmt/hsakmt.h"
#include "../libhsakmt/linux/kfd_ioctl.h"

using namespace std;


#define kmt_test_open() do{\
	kfd_fd = open(KFD_DEVICE, O_RDWR | O_CLOEXEC);\
	if (kfd_fd != -1)\
		printf("opened kfd %s, fd = %d.\n", KFD_DEVICE, kfd_fd);\
	else \
		printf("failed to open kfd %s.\n", KFD_DEVICE);\
	printf("\n");\
}while(0)

#define kmt_test_close() do{\
	close(kfd_fd);\
	printf("closed kfd fd = %d.\n", kfd_fd);\
	printf("\n");\
}while(0)

#define KFD_DEVICE					"/dev/kfd"
#define DRM_RENDER_PATH				"/dev/dri"
#define DRM_RENDER_DEVICE(drm_minor)	string(std::string("/dev/dri/renderD") + std::to_string(drm_minor)).data()
#define KFD_GENERATION_ID			"/sys/devices/virtual/kfd/kfd/topology/generation_id"
#define KFD_SYSTEM_PROPERTIES		"/sys/devices/virtual/kfd/kfd/topology/system_properties"
#define KFD_NODES_PATH				"/sys/devices/virtual/kfd/kfd/topology/nodes"
#define KFD_NODES(n)				string(std::string("/sys/devices/virtual/kfd/kfd/topology/nodes/") + std::to_string(n)).data()
#define PROC_CPUINFO_PATH			"/proc/cpuinfo"

extern int kfd_fd;
extern int gGpuId;
extern int gKmtNodeNum;
extern void kmt_test();
extern void kmt_info_test();
extern void kmt_mem_test();

extern void ff_kmt_alloc_host_cpu(void ** alloc_addr, size_t alloc_size, HsaMemFlags mem_flag);
extern void ff_kmt_free_host_cpu(void * mem_addr, size_t mem_size);

extern int kmtIoctl(int fd, unsigned long request, void *arg);
extern int readIntKey(std::string file, std::string key = "");
