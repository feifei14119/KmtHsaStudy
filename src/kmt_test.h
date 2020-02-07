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

extern int kfd_fd;
extern int gGpuId;
extern int gKmtNodeNum;
extern int sys_page_size;
extern void kmt_test();
extern void kmt_info_test();

extern int kmtIoctl(int fd, unsigned long request, void *arg);
extern uint64_t readIntKey(std::string file, std::string key = "");

// ==================================================================
// red-black tree
// ==================================================================
typedef struct rbtree_key_s 
{
#define ADDR_BIT 0
#define SIZE_BIT 1
	unsigned long addr;
	unsigned long size;
}rbtree_key_t;
typedef struct rbtree_node_s 
{
	rbtree_key_t    key;
	rbtree_node_s   *left;
	rbtree_node_s   *right;
	rbtree_node_s   *parent;
	unsigned char   color;
	unsigned char   data;
}rbtree_node_t;
typedef struct rbtree_s 
{
	rbtree_node_t   *root;
	rbtree_node_t   sentinel;
}rbtree_t;

// ==================================================================
// fmm
// ==================================================================
typedef struct vm_area
{
	void *start;
	void *end;
	struct vm_area *next;
	struct vm_area *prev;
}vm_area_t;
typedef struct
{
	void *start;
	void *userptr;
	uint64_t userptr_size;
	uint64_t size;	// size allocated on GPU. When the user requests a random
					// size, Thunk aligns it to page size and allocates this
					// aligned size on GPU

	uint64_t handle; // opaque
	uint32_t node_id;
	rbtree_node_t node;
	rbtree_node_t user_node;

	uint32_t flags; // memory allocation flags

	/* Registered nodes to map on SVM mGPU */
	uint32_t *registered_device_id_array;
	uint32_t registered_device_id_array_size;
	uint32_t *registered_node_id_array;
	uint32_t registration_count; // the same memory region can be registered multiple times

	/* Nodes that mapped already */
	uint32_t *mapped_device_id_array;
	uint32_t mapped_device_id_array_size;
	uint32_t *mapped_node_id_array;
	uint32_t mapping_count;

	void *metadata;//Metadata of imported graphics buffers
	void *user_data;//User data associated with the memory
	bool is_imported_kfd_bo;//Flag to indicate imported KFD buffer
}vm_object_t;
// ------------------------------------------------------------------
typedef struct
{
	void *base;
	void *limit;
} aperture_t;
typedef struct
{
	void *base;
	void *limit;
	uint64_t align;
	uint32_t guard_pages;
	vm_area_t *vm_ranges;
	rbtree_t tree;
	rbtree_t user_tree;
	//	pthread_mutex_t fmm_mutex;
	bool is_cpu_accessible;
	//	const manageable_aperture_ops_t *ops;
} manageable_aperture_t;
// ------------------------------------------------------------------
extern void kmt_mem_test();

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

// ==================================================================
// queue
// ==================================================================
struct process_doorbells {
	bool use_gpuvm;
	uint32_t size;
	void *mapping;
	pthread_mutex_t mutex;
};
extern void kmt_queue_test();
