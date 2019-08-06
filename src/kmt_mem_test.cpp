#include <string>
#include <iostream>
#include <vector>
#include <sys/mman.h>

#include "kmt_test.h"


void ff_kmt_alloc_host_cpu(void ** alloc_addr, size_t alloc_size, HsaMemFlags mem_flag)
{
	int mmap_prot = PROT_READ;
	
	if (mem_flag.ui32.ExecuteAccess)
		mmap_prot |= PROT_EXEC;

	if (!mem_flag.ui32.ReadOnly)
		mmap_prot |= PROT_WRITE;

	void *mem_addr = NULL;
	mem_addr = mmap(NULL, alloc_size, mmap_prot, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);

	*alloc_addr = mem_addr;

	printf("kmt alloc: %d(B) at 0x%X.\n", alloc_size, *alloc_addr);
	printf("kmt mmap_port: ");
	if(mmap_prot & PROT_READ)		printf("PROT_READ");
	if (mmap_prot & PROT_EXEC)		printf(" | PROT_EXEC");
	if (mmap_prot & PROT_WRITE)		printf(" | PROT_WRITE");
	printf("\n");
}
void ff_kmt_free_host_cpu(void * mem_addr, size_t mem_size)
{
	munmap(mem_addr, mem_size);
	printf("kmt free host cpu memory: %d(B) at 0x%X.\n", mem_size, mem_addr);
	printf("\n");
}

int drm_render_fd;
void kmt_init_drm_test()
{
	printf("=======================\n");
	printf("= drm render init     =\n");
	printf("=======================\n");
	int drm_render_minor;
	// 只考虑GPU节点, CPU节点支持SYSFS, 但不打开DRM
	// num_sysfs_nodes = 系统所有节点数; drm_render_fds 只记录GPU节点
	drm_render_minor = readIntKey(KFD_NODES(1) + string("/properties"), "drm_render_minor");
	printf("gpu node 1 drm_render_minor = %d.\n", drm_render_minor);
	drm_render_fd = open(DRM_RENDER_DEVICE(drm_render_minor), O_RDWR | O_CLOEXEC);
	printf("open node 1 drm device %s. fd = %d\n", DRM_RENDER_DEVICE(drm_render_minor), drm_render_fd);
	printf("\n");
}
void kmt_close_drm_test()
{
	close(drm_render_fd);
	printf("close node 1 drm device. fd = %d\n", drm_render_fd);
	printf("\n");
}

int aperture_num;
struct kfd_process_device_apertures * apertures;
void kmt_aperture_test()
{
	printf("=======================\n");
	printf("= kmt aperture init   =\n");
	printf("=======================\n");

	apertures = (kfd_process_device_apertures*)calloc(gKmtNodeNum, sizeof(struct kfd_process_device_apertures));

	struct kfd_ioctl_get_process_apertures_new_args args_new = { 0 };
	args_new.kfd_process_device_apertures_ptr = (uintptr_t)apertures;
	args_new.num_of_nodes = gKmtNodeNum; // gpu资源管理可能会禁止节点
	int rtn = kmtIoctl(kfd_fd, AMDKFD_IOC_GET_PROCESS_APERTURES_NEW, (void *)&args_new);

	if (rtn == 0)
	{
		aperture_num = args_new.num_of_nodes;
	}
	else
	{
		printf("can't use new ioctl, using old ioctl.\n");
		struct kfd_ioctl_get_process_apertures_args args_old;
		printf("old ioctl support aperture number = %d.\n", NUM_OF_SUPPORTED_GPUS);
		memset(&args_old, 0, sizeof(args_old));
		kmtIoctl(kfd_fd, AMDKFD_IOC_GET_PROCESS_APERTURES, (void *)&args_old);
		memcpy(apertures, args_old.process_apertures, sizeof(*apertures) * args_old.num_of_nodes);
		aperture_num = args_old.num_of_nodes;
	}

	printf("process apertures number = %d.\n", aperture_num);
	for (int i = 0; i < aperture_num; i++)
	{
		printf("\t-----------------------\n");
		printf("\tprocess apertures = %d.\n", i);
		printf("\tgpu_id = %d.\n", apertures[i].gpu_id);
		printf("\tpad = %d.\n", apertures[i].pad);
		printf("\tgpuvm_base = 0x%X.\n", apertures[i].gpuvm_base);
		printf("\tgpuvm_limit = %ld(TB).\n", apertures[i].gpuvm_limit / 1024 / 1024 / 1024 / 1024);
		printf("\tlds_base = 0x%X.\n", apertures[i].lds_base);
		printf("\tlds_limit = %ld(TB).\n", apertures[i].lds_limit / 1024 / 1024 / 1024 / 1024);
		printf("\tscratch_base = 0x%X.\n", apertures[i].scratch_base);
		printf("\tscratch_limit = %ld(TB).\n", apertures[i].scratch_limit / 1024 / 1024 / 1024 / 1024);
	}

	struct kfd_ioctl_get_process_apertures_args args_old;
}

void kmt_vm_test()
{
	printf("=======================\n");
	printf("= kmt vm init         =\n");
	printf("=======================\n");
	//aperture_allocate_area()

	struct kfd_ioctl_acquire_vm_args args;
	args.gpu_id = gGpuId;
	args.drm_fd = drm_render_fd;
	printf("acquire vm from drm render node. gpu_id = %d, drm_fs = %d\n", gGpuId, drm_render_fd);
	int rtn = kmtIoctl(kfd_fd, AMDKFD_IOC_ACQUIRE_VM, (void *)&args);
	if (rtn != 0)
		printf("failed to acquire vm from drm render node. err = %d\n", rtn);
	printf("acquire vm from drm render node.\n");
}

void kmt_mem_test()
{
	printf("***********************\n");
	printf("* kmt memory test     *\n");
	printf("***********************\n");

	kmt_init_drm_test();
	kmt_aperture_test();
	kmt_vm_test();

	kmt_close_drm_test();
	
}
