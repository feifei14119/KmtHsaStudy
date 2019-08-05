#pragma once

#include "../libhsakmt/hsakmttypes.h"
#include "../libhsakmt/hsakmt.h"
#include "../libhsakmt/linux/kfd_ioctl.h"

extern void kmt_test();

#define kmt_test_open() do{\
	kfd_fd = open(kfd_device_name, O_RDWR | O_CLOEXEC);\
	if (kfd_fd != -1)\
		printf("opened kfd %s, fd = %d.\n", kfd_device_name, kfd_fd);\
	else \
		printf("failed to open kfd %s.\n", kfd_device_name);\
}while(0)

#define kmt_test_close() do{\
	close(kfd_fd);\
	printf("closed kfd fd = %d.\n", kfd_fd);\
}while(0)

extern int kmtIoctl(int fd, unsigned long request, void *arg);

extern void ff_kmt_alloc_host_cpu(void ** alloc_addr, size_t alloc_size, HsaMemFlags mem_flag);
extern void ff_kmt_free_host_cpu(void * mem_addr, size_t mem_size);
