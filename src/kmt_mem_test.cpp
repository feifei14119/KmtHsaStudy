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


// ==================================================================
typedef struct manageable_aperture 
{
	void *base;
	void *limit;
	uint64_t align;
	uint32_t guard_pages;
//	vm_area_t *vm_ranges;
//	rbtree_t tree;
//	rbtree_t user_tree;
//	pthread_mutex_t fmm_mutex;
	bool is_cpu_accessible;
//	const manageable_aperture_ops_t *ops;
} manageable_aperture_t;
// ------------------------------------------------------------------
typedef struct 
{
	void *base;
	void *limit;
} aperture_t;

// ==================================================================
int aperture_num;
struct kfd_process_device_apertures * kfd_dev_aperture;
manageable_aperture_t cpuvm_aperture;
// ------------------------------------------------------------------
// gpu_mem_t * gpu_mem
int drm_render_fd;
aperture_t lds_aperture;
aperture_t scratch_aperture;
aperture_t mmio_aperture;
manageable_aperture_t gpuvm_aperture;
manageable_aperture_t scratch_physical;
// ------------------------------------------------------------------
// svm_t svm
bool disable_cache = false;
manageable_aperture_t dgpu_aperture;
manageable_aperture_t dgpu_alt_aperture;

// ==================================================================
// ==================================================================
void kmt_init_drm_test()
{
	printf("=======================\n");
	printf("open drm render.\n");
	// 只考虑GPU节点, CPU节点支持SYSFS, 但不打开DRM
	// num_sysfs_nodes = 系统所有节点数; drm_render_fds 只记录GPU节点
	int drm_render_minor = readIntKey(KFD_NODES(1) + string("/properties"), "drm_render_minor");
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

// ==================================================================
// ==================================================================
void kmt_aperture_test()
{
	printf("=======================\n");
	printf("get kfd device aperture\n");

	kfd_dev_aperture = (kfd_process_device_apertures*)calloc(gKmtNodeNum, sizeof(struct kfd_process_device_apertures));

	struct kfd_ioctl_get_process_apertures_new_args args_new = { 0 };
	args_new.kfd_process_device_apertures_ptr = (uintptr_t)kfd_dev_aperture;
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
		printf("old ioctl support max aperture number = %d.\n", NUM_OF_SUPPORTED_GPUS);
		memset(&args_old, 0, sizeof(args_old));
		kmtIoctl(kfd_fd, AMDKFD_IOC_GET_PROCESS_APERTURES, (void *)&args_old);
		memcpy(kfd_dev_aperture, args_old.process_apertures, sizeof(*kfd_dev_aperture) * args_old.num_of_nodes);
		aperture_num = args_old.num_of_nodes;
	}

	printf("kfd gpu device apertures number = %d.\n", aperture_num);
	for (int i = 0; i < aperture_num; i++)
	{
		printf("\t-----------------------\n");
		printf("\tkfd gpu device apertures = %d.\n", i);
		printf("\tgpu_id = %d.\n", kfd_dev_aperture[i].gpu_id);
		printf("\tpad = %d.\n", kfd_dev_aperture[i].pad);
		printf("\tgpuvm_base = 0x%016lX.\n", kfd_dev_aperture[i].gpuvm_base);
		printf("\tgpuvm_limit = %ld(TB).\n", kfd_dev_aperture[i].gpuvm_limit / 1024 / 1024 / 1024 / 1024);
		printf("\tlds_base = 0x%016lX.\n", kfd_dev_aperture[i].lds_base);
		printf("\tlds_limit = %ld(TB).\n", kfd_dev_aperture[i].lds_limit / 1024 / 1024 / 1024 / 1024);
		printf("\tscratch_base = 0x%016lX.\n", kfd_dev_aperture[i].scratch_base);
		printf("\tscratch_limit = %ld(TB).\n", kfd_dev_aperture[i].scratch_limit / 1024 / 1024 / 1024 / 1024);
	}

	printf("\n");
}

// ==================================================================
// ==================================================================
void * mmap_aperture_allocate_aligned(uint64_t size);
#define GFX9_LIMIT				(1ULL << 47)
#define IS_CANONICAL_ADDR(a)	((a) < (1ULL << 47))
#define GPU_HUGE_PAGE_SIZE		(2 << 20)
#define SVM_MIN_VM_SIZE			(4ULL << 30)
#define SVM_RESERVATION_LIMIT	((1ULL << 40) - 1)
#define VOID_PTR_ADD(ptr,n) (void*)((uint8_t*)(ptr) + n)/*ptr + offset*/
#define VOID_PTRS_SUB(ptr1,ptr2) (uint64_t)((uint8_t*)(ptr1) - (uint8_t*)(ptr2)) /*ptr1 - ptr2*/
void kmt_init_vm_test()
{
	printf("=======================\n");
	printf("acquire VM from DRM render node for KFD use\n");
	struct kfd_ioctl_acquire_vm_args args_vm;
	args_vm.gpu_id = kfd_dev_aperture->gpu_id;
	args_vm.drm_fd = drm_render_fd;
	printf("acquire vm from drm render. gpu_id = %d, drm_fs = %d\n", kfd_dev_aperture->gpu_id, drm_render_fd);
	int rtn = kmtIoctl(kfd_fd, AMDKFD_IOC_ACQUIRE_VM, (void *)&args_vm);
	if (rtn != 0)
		printf("Failed to acquire vm from drm render. err = %d\n", rtn);
	printf("\n");

	printf("=======================\n");
	printf("init_svm_apertures():reserve SVM address\n");
	int svm_alig = sys_page_size;
	int guard_page = 1;
	uint64_t svm_base = kfd_dev_aperture->gpuvm_base;
	uint64_t svm_limt = kfd_dev_aperture->gpuvm_limit;
	svm_base = ALIGN_UP(svm_base, GPU_HUGE_PAGE_SIZE);
	svm_limt = ((svm_limt + 1) & ~(size_t)(GPU_HUGE_PAGE_SIZE - 1)) - 1;
	printf("gpuvm: base = 0x%lX, limit = 0x%lX, page_size = %d, guard_page = %d.\n", svm_base, svm_limt, svm_alig, guard_page);
	printf("init_mmap_apertures():\n");
	dgpu_aperture.base = (void*)svm_base;
	dgpu_aperture.limit = (void*)svm_limt;
	dgpu_aperture.align = svm_alig;
	dgpu_aperture.guard_pages = guard_page;
	dgpu_aperture.is_cpu_accessible = true;
	dgpu_alt_aperture = dgpu_aperture;
	printf("dgpu_aperture: base = 0x%lX, limit = 0x%lX.\n", dgpu_aperture.base, dgpu_aperture.limit);
	printf("dgpu_alt_aperture: base = 0x%lX, limit = 0x%lX.\n", dgpu_alt_aperture.base, dgpu_alt_aperture.limit);
	printf("\n");

//	printf("try to allocate one page.\n");
//	void * try_addr;
//	try_addr = mmap_aperture_allocate_aligned(sys_page_size);
//	printf("try address = 0x%016lX.\n", try_addr);

	// TODO: vm map aperture
#if 0
	if (svm_limt > SVM_RESERVATION_LIMIT)
		svm_limt = SVM_RESERVATION_LIMIT;
	printf("gpuvm: base = 0x%lX, limit = 0x%lX.\n", svm_base, svm_limt);
	printf("\n");

	printf("\t-----------------------\n");
	printf("\tTry to reserve address space for SVM.\n");
	size_t len, map_size; bool found = false;
	void * addr, * ret_addr;
	size_t top;
	const size_t ADDR_INC = GPU_HUGE_PAGE_SIZE;
	for (len = svm_limt - svm_base + 1; !found && len >= SVM_MIN_VM_SIZE; len = (len + 1) >> 1)
	{
		printf("\t\tlen=0x%lX.\n", len);
		for (addr = (void *)svm_base, ret_addr = NULL;
			(uint64_t)addr + ((len + 1) >> 1) - 1 <= svm_limt;
			addr = (void *)((uint64_t)addr + ADDR_INC))
		{
			if((size_t)addr + len > svm_limt + 1)
				top = svm_limt + 1;
			else
				top = (size_t)addr + len;

			map_size = (top - (size_t)addr) & ~(size_t)(sys_page_size - 1);
			if (map_size < SVM_MIN_VM_SIZE)
				break;

			printf("\t\tmap addr = 0x%lX, size = %ld(GB).\n", addr, map_size / 1024 / 1024 / 1024);
			ret_addr = mmap(addr, map_size, PROT_NONE, MAP_ANONYMOUS | MAP_NORESERVE | MAP_PRIVATE, -1, 0);
			if (ret_addr == MAP_FAILED)
				break;
			if ((size_t)ret_addr + ((len + 1) >> 1) - 1 <= svm_limt)
				break;
			printf("\t\trtn addr = 0x%lX.\n", ret_addr);
			printf("\t\tump addr = 0x%lX, size = %ld(GB).\n", ret_addr, map_size / 1024 / 1024 / 1024);
			munmap(ret_addr, map_size);
			ret_addr = NULL;
		}
		if (!ret_addr)
		{
			printf("\t\tFailed to reserve %uGB for SVM ...\n", (unsigned int)(len >> 30));
			continue;
		}

		if ((size_t)ret_addr + SVM_MIN_VM_SIZE - 1 > svm_limt)
		{
			/* addressable size is less than the minimum */
			printf("\t\tGot %uGB for SVM at %p with only %dGB usable ...\n", (unsigned int)(map_size >> 30), ret_addr, (int)((svm_limt - (size_t)ret_addr) >> 30));
			munmap(ret_addr, map_size);
			ret_addr = NULL;
			continue;
		}
		else
		{
			found = true;
			break;
		}
	}

	if (found)
	{
		printf("\tSuccess found reserve %uGB for SVM at 0x%lX.\n", (unsigned int)(len >> 30), ret_addr);
	}
	printf("\n");
	
	svm_base = (uint64_t)ret_addr;
	if (svm_base + map_size - 1 > svm_limt)
		munmap((void *)(svm_limt + 1), svm_base + map_size - 1 - svm_limt);
	else
		svm_limt = svm_base + map_size - 1;

	printf("\t-----------------------\n");
	printf("\tset svm default dpu_aperture.\n");
	dgpu_aperture.base = ret_addr;
	dgpu_aperture.limit = (void*)svm_limt;
	dgpu_aperture.align = svm_alig;
	dgpu_aperture.guard_pages = guard_page;
	dgpu_aperture.is_cpu_accessible = true;
	//dgpu_aperture.ops = ;

	printf("\tset svm coherent dpu_aperture.\n");
	uint64_t alt_base, alt_size;
	alt_base = (uint64_t)dgpu_aperture.base;
	alt_size = (VOID_PTRS_SUB(dgpu_aperture.limit, dgpu_aperture.base) + 1) >> 2;
	alt_base = (alt_base + 0xffff) & ~0xffffULL;
	alt_size = (alt_size + 0xffff) & ~0xffffULL;
	dgpu_alt_aperture.base = (void *)alt_base;
	dgpu_alt_aperture.limit = (void *)(alt_base + alt_size - 1);
	dgpu_alt_aperture.align = svm_alig;
	dgpu_alt_aperture.guard_pages = guard_page;
	dgpu_alt_aperture.is_cpu_accessible = true;
	//dgpu_alt_aperture.ops = ;

	dgpu_aperture.base = VOID_PTR_ADD(dgpu_alt_aperture.limit, 1);
	printf("\tSVM alt (coherent): 0x%016lX - 0x%016lX\n", dgpu_alt_aperture.base, dgpu_alt_aperture.limit);
	printf("\tSVM (non-coherent): 0x%016lX - 0x%016lX\n", dgpu_aperture.base, dgpu_aperture.limit);
	printf("\n");
#endif

	printf("=======================\n");
	printf("fmm_set_memory_policy()\n");
	struct kfd_ioctl_set_memory_policy_args args_plc = { 0 };
	args_plc.gpu_id = kfd_dev_aperture->gpu_id;
	args_plc.default_policy = disable_cache ? KFD_IOC_CACHE_POLICY_COHERENT : KFD_IOC_CACHE_POLICY_NONCOHERENT;
	args_plc.alternate_policy = KFD_IOC_CACHE_POLICY_COHERENT;
	args_plc.alternate_aperture_base = (uint64_t)dgpu_alt_aperture.base;
	args_plc.alternate_aperture_size = VOID_PTRS_SUB(dgpu_alt_aperture.limit, dgpu_alt_aperture.base) + 1;
	rtn = kmtIoctl(kfd_fd, AMDKFD_IOC_SET_MEMORY_POLICY, &args_plc);
	if (rtn == 0)
		printf("set memory policy base = 0x%016lX, size = %ld(GB).\n", args_plc.alternate_aperture_base, args_plc.alternate_aperture_size / 1024 / 1024 / 1024);
	printf("\n");

	// ----------------------------------------------
	/*
	aperture_t:
	lds_aperture.base = (void*)kfd_dev_aperture[0].lds_base;
	lds_aperture.limit = (void*)kfd_dev_aperture[0].lds_limit;

	scratch_aperture.base = (void*)kfd_dev_aperture[0].scratch_base;
	scratch_aperture.limit = (void*)kfd_dev_aperture[0].scratch_limit;


	manageable_aperture_t:
	scratch_physical.align = sys_page_size;
	scratch_physical.ops = reserved_aperture_ops

	gpuvm_aperture.base = NULL;
	gpuvm_aperture.limit = NULL;
	gpuvm_aperture.align = sys_page_size;
	gpuvm_aperture.guard_pages = guard_page;
	
	cpuvm_aperture.base = 0;
	cpuvm_aperture.limit = (void *)0x7FFFFFFFFFFF; // 128T
	cpuvm_aperture.align = sys_page_size;
	*/
	// ----------------------------------------------
}

// ==================================================================
// ==================================================================
void * mmap_aperture_allocate_aligned(uint64_t size)
{
	size_t align = dgpu_alt_aperture.align;
	int guard_pages = dgpu_alt_aperture.guard_pages;
	size_t guard_size, aligned_padded_size;
	void * addr, * aligned_addr;

	printf("/*\n");
	while (align < GPU_HUGE_PAGE_SIZE && size >= (align << 1))
	{
		align <<= 1;
		printf(".");
	}

	size = ALIGN_UP(size, align);
	guard_size = (uint64_t)guard_pages * sys_page_size;
	printf("        guard size = 0x%016lX.\n", guard_size);
	aligned_padded_size = size + align + 2 * guard_size - sys_page_size;
	printf("aligned guard size = 0x%016lX.\n", aligned_padded_size);

	addr = mmap(0, aligned_padded_size, PROT_NONE, MAP_ANONYMOUS | MAP_NORESERVE | MAP_PRIVATE, -1, 0);
	printf("mmap %d(KB) at 0x%016lX.\n", aligned_padded_size / 1024, addr);
	aligned_addr = (void *)ALIGN_UP((uint64_t)addr + guard_size, align);
	printf("aligned addr = 0x%016lX.\n", aligned_addr);

	if (aligned_addr < dgpu_alt_aperture.base ||
		VOID_PTR_ADD(aligned_addr, size - 1) > dgpu_alt_aperture.limit)
	{
		printf("mmap returned %p, out of range %p-%p\n", aligned_addr, dgpu_alt_aperture.base, dgpu_alt_aperture.limit);
		munmap(addr, aligned_padded_size);
		return NULL;
	}

	if (aligned_addr > addr)
	{
		int err= munmap(addr, VOID_PTRS_SUB(aligned_addr, addr));
		printf("munmap 1: addr = 0x%016lX, aligned_addr = 0x%016lX. err = %d\n", addr, aligned_addr, err);
	}

	void *aligned_end, *mapping_end;
	aligned_end = VOID_PTR_ADD(aligned_addr, size);
	mapping_end = VOID_PTR_ADD(addr, aligned_padded_size);
	if (mapping_end > aligned_end)
	{
		int err = munmap(aligned_end, VOID_PTRS_SUB(mapping_end, aligned_end));
		printf("munmap 2: mapping_end = 0x%016lX, aligned_end = 0x%016lX. err = %d\n", mapping_end, aligned_end, err);
	}
	printf("*/\n");

	return aligned_addr;
}
void kmt_init_mmio_test()
{
	printf("=======================\n");
	printf("mmio address: map_mmio()\n");
	// dgpu_alt_aperture
	// dgpu_alt_aperture->ops = reserved_aperture_ops
	//								reserved_aperture_allocate_aligned
	//								reserved_aperture_release
	int err;
	void * addr, * mem;
	size_t memSize = sys_page_size;
	uint64_t mmap_offset;
	uint32_t ioc_flag = KFD_IOC_ALLOC_MEM_FLAGS_MMIO_REMAP|
		KFD_IOC_ALLOC_MEM_FLAGS_WRITABLE |
		KFD_IOC_ALLOC_MEM_FLAGS_COHERENT;
	// map_mmio()
	//     __fmm_allocate_device() // alloc page_size physical mem
	//         aperture_allocate_area()		// allocate address space
	//         fmm_allocate_memory_object()	// allocate in device: ioctrl
	//     mmap()				// map to cpu
	//     fmm_map_to_gpu()		// map to gpu
	printf("\t-----------------------\n");
	printf("\tfmm_allocate_device()\n");
	printf("\t\tmmap_aperture_allocate_aligned().\n");
	//printf("\t\t");
	memSize *= 1;
	addr = mmap_aperture_allocate_aligned(memSize);
	printf("\t\tallocate physical mem %d(KB) at 0x%016lX.\n", memSize/1024, addr);
	printf("\n");

	printf("\t\tfmm_allocate_memory_object().\n");
	struct kfd_ioctl_alloc_memory_of_gpu_args args_alloc = { 0 };
	args_alloc.gpu_id = kfd_dev_aperture->gpu_id;
	args_alloc.size = ALIGN_UP(memSize, dgpu_alt_aperture.align);
	args_alloc.flags = ioc_flag | KFD_IOC_ALLOC_MEM_FLAGS_NO_SUBSTITUTE;
	args_alloc.va_addr = (uint64_t)addr;
	err = kmtIoctl(kfd_fd, AMDKFD_IOC_ALLOC_MEMORY_OF_GPU, &args_alloc);
	if (err != 0)
		printf("\t\tFailed to alloc memory on gpu. err = %d.\n", err);
	else
		printf("\t\tAlloc memory on gpu.\n");
	printf("\n");

	mmap_offset = args_alloc.mmap_offset;
	printf("\t\tmmap_offset = 0x%016lX.\n", mmap_offset);

	printf("\t\t\taperture_allocate_object().\n");

	printf("\t-----------------------\n");
	printf("\tmap to cpu\n");
	printf("\tmmap for cpu access %d(KB) at 0x%016lX.\n", sys_page_size/1024, addr);
	mem = mmap(addr, sys_page_size, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_FIXED, drm_render_fd, mmap_offset);
	if (mem == MAP_FAILED)
		printf("\tFailed to mmap for cpu access.\n", mem);
	printf("\n");

	printf("\t-----------------------\n");
	printf("\tmap to gpu: fmm_map_to_gpu()\n");
	printf("\n");	
}


// ==================================================================
// ==================================================================
void kmt_mem_test()
{
	printf("***********************\n");
	printf("* kmt memory test     *\n");
	printf("***********************\n");

	kmt_init_drm_test();
	kmt_aperture_test();
	kmt_init_vm_test();
	kmt_init_mmio_test();

	kmt_close_drm_test();	
}
