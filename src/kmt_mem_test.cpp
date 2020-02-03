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


typedef struct vm_area 
{
	void *start;
	void *end;
	struct vm_area *next;
	struct vm_area *prev;
}vm_area_t;
typedef struct vm_object
{
	void *start;
	void *userptr;
	uint64_t userptr_size;
	uint64_t size;	// size allocated on GPU. When the user requests a random
					// size, Thunk aligns it to page size and allocates this
					// aligned size on GPU

	uint64_t handle; /* opaque */
	uint32_t node_id;
	//rbtree_node_t node;
	//rbtree_node_t user_node;

	uint32_t flags; /* memory allocation flags */
	/* Registered nodes to map on SVM mGPU */
	uint32_t *registered_device_id_array;
	uint32_t registered_device_id_array_size;
	uint32_t *registered_node_id_array;
	uint32_t registration_count; /* the same memory region can be registered multiple times */
	/* Nodes that mapped already */
	uint32_t *mapped_device_id_array;
	uint32_t mapped_device_id_array_size;
	uint32_t *mapped_node_id_array;
	uint32_t mapping_count;
	/* Metadata of imported graphics buffers */
	void *metadata;
	/* User data associated with the memory */
	void *user_data;
	/* Flag to indicate imported KFD buffer */
	bool is_imported_kfd_bo;
}vm_object_t;

// ==================================================================
typedef struct manageable_aperture 
{
	void *base;
	void *limit;
	uint64_t align;
	uint32_t guard_pages;
	vm_area_t *vm_ranges;
//	rbtree_t tree;
//	rbtree_t user_tree;
//	pthread_mutex_t fmm_mutex;
	bool is_cpu_accessible;
//	const manageable_aperture_ops_t *ops;
} manageable_aperture_t;
typedef struct 
{
	void *base;
	void *limit;
} aperture_t;

// ==================================================================

// ------------------------------------------------------------------
// process apertures
int aperture_num;
struct kfd_process_device_apertures * kfd_dev_aperture;
// ------------------------------------------------------------------
// gpu_mem_t * gpu_mem
int drm_render_fd;
aperture_t lds_aperture;
aperture_t scratch_aperture;
aperture_t mmio_aperture;
manageable_aperture_t gpuvm_aperture;
manageable_aperture_t scratch_physical;
manageable_aperture_t cpuvm_aperture;
// ------------------------------------------------------------------
// svm_t svm
bool disable_cache = false;
manageable_aperture_t svm_dgpu_aperture;
manageable_aperture_t svm_dgpu_alt_aperture;

vm_object_t *vm_obj = NULL;
static uint32_t all_gpu_id_array_size;
static uint32_t *all_gpu_id_array;

// ==================================================================
// ==================================================================
void open_drm()
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
void acquire_vm_from_drm()
{
	printf("=======================\n");
	printf("acquire VM from DRM render node for KFD use\n");
	struct kfd_ioctl_acquire_vm_args args_vm;
	args_vm.gpu_id = gGpuId;
	args_vm.drm_fd = drm_render_fd;
	int rtn = kmtIoctl(kfd_fd, AMDKFD_IOC_ACQUIRE_VM, (void *)&args_vm);
	if (rtn != 0)
		printf("Failed to acquire vm from drm render. err = %d\n", rtn);
	printf("\n");
}
void close_drm()
{
	printf("=======================\n");
	close(drm_render_fd);
	printf("close node 1 drm device. fd = %d\n", drm_render_fd);
	printf("\n");
}

// ==================================================================
// ==================================================================
void get_process_apertures()
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
		printf("use new ioctl to get aperture.\n");
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
		printf("\tprocess apertures %d:\n", i);
		printf("\tgpu_id = %d.\n", kfd_dev_aperture[i].gpu_id);
		printf("\tpad = %d.\n", kfd_dev_aperture[i].pad);
		printf("\tgpuvm   base = 0x%016lX.\tlimit = %ld(TB).\n", kfd_dev_aperture[i].gpuvm_base, kfd_dev_aperture[i].gpuvm_limit / 1024 / 1024 / 1024 / 1024);
		printf("\tlds     base = 0x%016lX.\tlimit = %ld(TB).\n", kfd_dev_aperture[i].lds_base, kfd_dev_aperture[i].lds_limit / 1024 / 1024 / 1024 / 1024);
		printf("\tscratch base = 0x%016lX.\tlimit = %ld(TB).\n", kfd_dev_aperture[i].scratch_base, kfd_dev_aperture[i].scratch_limit / 1024 / 1024 / 1024 / 1024);
	}

	printf("\n");
}

// ==================================================================
// ==================================================================
#define GFX9_LIMIT				(1ULL << 47)
#define IS_CANONICAL_ADDR(a)	((a) < (1ULL << 47))
#define GPU_HUGE_PAGE_SIZE		(2 << 20)
#define SVM_MIN_VM_SIZE			(4ULL << 30)
#define SVM_RESERVATION_LIMIT	((1ULL << 40) - 1)
#define VOID_PTR_ADD(ptr,n) (void*)((uint8_t*)(ptr) + n)/*ptr + offset*/
#define VOID_PTRS_SUB(ptr1,ptr2) (uint64_t)((uint8_t*)(ptr1) - (uint8_t*)(ptr2)) /*ptr1 - ptr2*/
void init_svm_apertures()
{
	printf("=======================\n");
	printf("init svm apertures\n");
	uint32_t guard_page = 1;
	uint32_t svm_align = sys_page_size;
	uint64_t svm_base = kfd_dev_aperture->gpuvm_base;
	uint64_t svm_limt = kfd_dev_aperture->gpuvm_limit;
	printf("\tsvm   base = 0x%016lX.\tlimit = %ld(TB).\talign = %ld(KB)\n", svm_base, svm_limt / 1024 / 1024 / 1024 / 1024, svm_align / 1024);
	svm_base = ALIGN_UP(svm_base, GPU_HUGE_PAGE_SIZE);
	svm_limt = ((svm_limt + 1) & ~(size_t)(GPU_HUGE_PAGE_SIZE - 1)) - 1;
	svm_dgpu_aperture.base = (void*)svm_base;
	svm_dgpu_aperture.limit = (void*)svm_limt;
	svm_dgpu_aperture.align = svm_align;
	svm_dgpu_aperture.guard_pages = guard_page;
	svm_dgpu_aperture.is_cpu_accessible = true;

	uint64_t alt_base = svm_base;
	uint64_t alt_size = (VOID_PTRS_SUB(svm_limt, svm_base) + 1) >> 2;
	alt_base = (alt_base + 0xffff) & ~0xffffULL;
	alt_size = (alt_size + 0xffff) & ~0xffffULL;
	svm_dgpu_alt_aperture.base = (void*)alt_base;
	svm_dgpu_alt_aperture.limit = (void*)alt_size;
	svm_dgpu_alt_aperture.align = svm_align;
	svm_dgpu_alt_aperture.guard_pages = guard_page;
	svm_dgpu_alt_aperture.is_cpu_accessible = true;

	svm_dgpu_aperture.base = VOID_PTR_ADD(alt_size, 1);
	printf("\tdgpu  base = 0x%016lX.\tlimit = %ld(TB).\talign = %ld(KB)\n", svm_base, svm_limt / 1024 / 1024 / 1024 / 1024, svm_align / 1024);
	printf("\talt   base = 0x%016lX.\tlimit = %ld(TB).\t\talign = %ld(KB)\n", alt_base, alt_size / 1024 / 1024 / 1024 / 1024, svm_align / 1024);
	printf("\n");
}

// ==================================================================
// ==================================================================
uint64_t mmap_offset; void * mmio_addr;
vm_area_t *vm_create_and_init_area(void *start, void *end)
{
	vm_area_t *area = (vm_area_t *)malloc(sizeof(vm_area_t));

	if (area)
	{
		area->start = start;
		area->end = end;
		area->next = area->prev = NULL;
	}

	return area;
}
void *aperture_allocate_area(manageable_aperture_t *app, void *address,	uint64_t MemorySizeInBytes,	uint64_t align)
{	
	// alt's opt is	reserved_aperture_allocate_aligned()
	printf("\t1. create vm_area_t for aperture\n");

	vm_area_t *cur, *next;
	void *start;

	if (align < app->align)
		align = app->align;

	/* Align big buffers to the next power-of-2 up to huge page
	 * size for flexible fragment size TLB optimizations
	 */
	while (align < GPU_HUGE_PAGE_SIZE && MemorySizeInBytes >= (align << 1))
		align <<= 1;
	
	// Leave at least one guard page after every object to catch
	MemorySizeInBytes = ALIGN_UP(ALIGN_UP(MemorySizeInBytes, app->align) + (uint64_t)app->guard_pages * sys_page_size, app->align);

	/* Find a big enough "hole" in the address space */
	cur = NULL;
	next = app->vm_ranges;
	start = address ? address : (void *)ALIGN_UP((uint64_t)app->base, align);
	while (next)
	{
		if (next->start > start &&
			VOID_PTRS_SUB(next->start, start) >= MemorySizeInBytes)
			break;

		cur = next;
		next = next->next;
		if (!address)
			start = (void *)ALIGN_UP((uint64_t)cur->end + 1, align);
	}
	if (!next && VOID_PTRS_SUB(app->limit, start) + 1 < MemorySizeInBytes)
		/* No hole found and not enough space after the last area */
		return NULL;

	if (cur && address && address < (void *)ALIGN_UP((uint64_t)cur->end + 1, align))
		/* Required address is not free or overlaps */
		return NULL;

	if (cur && VOID_PTR_ADD(cur->end, 1) == start)
	{
		/* extend existing area */
		cur->end = VOID_PTR_ADD(start, MemorySizeInBytes - 1);
	}
	else
	{
		vm_area_t *new_area;
		/* create a new area between cur and next */

		new_area = vm_create_and_init_area(start, VOID_PTR_ADD(start, (MemorySizeInBytes - 1)));

		if (!new_area)
			return NULL;
		new_area->next = next;
		new_area->prev = cur;

		if (cur)
			cur->next = new_area;
		else
			app->vm_ranges = new_area;
		if (next)
			next->prev = new_area;

		printf("\t\tstart = 0x%016lX\n", new_area->start);
		printf("\t\tsize  = 0x%016lX = %ld(KB)\n", MemorySizeInBytes, MemorySizeInBytes / 1024);
		printf("\t\tend   = 0x%016lX\n", new_area->end);
		printf("\n");
	}

	return start;
}

uint64_t kfd_allocate_memory(manageable_aperture_t *app, void *mem, uint64_t MemorySizeInBytes, uint32_t flags)
{
	printf("\t2. alloc memory on gpu \n");
	struct kfd_ioctl_alloc_memory_of_gpu_args args = { 0 };

	/* Allocate memory from amdkfd */
	args.gpu_id = gGpuId;
	args.size = ALIGN_UP(MemorySizeInBytes, app->align);
	args.flags = flags | KFD_IOC_ALLOC_MEM_FLAGS_NO_SUBSTITUTE;
	args.va_addr = (uint64_t)mem;
	//if (!topology_is_dgpu(get_device_id_by_gpu_id(gGpuId)) && (flags & KFD_IOC_ALLOC_MEM_FLAGS_VRAM))
	//	args.va_addr = VOID_PTRS_SUB(mem, app->base);
	if (flags & KFD_IOC_ALLOC_MEM_FLAGS_USERPTR)
		args.mmap_offset = mmap_offset;

	kmtIoctl(kfd_fd, AMDKFD_IOC_ALLOC_MEMORY_OF_GPU, &args);

	printf("\t\tmemory = 0x%016lX\n", mem);
	printf("\t\tusrptr = 0x%016lX\n", mmap_offset);
	printf("\t\toffset = 0x%016lX\n", args.mmap_offset);
	printf("\t\thandle = 0x%016lX\n", args.handle);
	printf("\n");

	mmap_offset = args.mmap_offset;
	return args.handle;
}

vm_object_t *vm_create_and_init_object(void *start, uint64_t size,	uint64_t handle, uint32_t flags)
{
	vm_object_t *object = (vm_object_t *)malloc(sizeof(vm_object_t));

	if (object)
	{
		object->start = start;
		object->userptr = NULL;
		object->userptr_size = 0;
		object->size = size;
		object->handle = handle;
		object->registered_device_id_array_size = 0;
		object->mapped_device_id_array_size = 0;
		object->registered_device_id_array = NULL;
		object->mapped_device_id_array = NULL;
		object->registered_node_id_array = NULL;
		object->mapped_node_id_array = NULL;
		object->registration_count = 0;
		object->mapping_count = 0;
		object->flags = flags;
		object->metadata = NULL;
		object->user_data = NULL;
		object->is_imported_kfd_bo = false;
//		object->node.key = rbtree_key((unsigned long)start, size);
//		object->user_node.key = rbtree_key(0, 0);
	}

	return object;
}
vm_object_t *fmm_allocate_memory_object(manageable_aperture_t *app, void *mem, uint64_t MemorySizeInBytes, uint64_t handle, uint32_t flags)
{
	printf("\t3. create vm_object_t for alloced memory\n");

	//pthread_mutex_lock(&app->fmm_mutex);
	vm_object_t *new_object;
	MemorySizeInBytes = ALIGN_UP(MemorySizeInBytes, app->align);
	new_object = vm_create_and_init_object(mem, MemorySizeInBytes, handle, flags);
	//pthread_mutex_unlock(&app->fmm_mutex);

	printf("\t\tstart  = 0x%016lX\n", new_object->start);
	printf("\t\tsize   = 0x%016lX\n", new_object->size);
	printf("\t\thandle = 0x%016lX\n", new_object->handle);
	printf("\n");
	return new_object;
}

void * allocate_device()
{
	/*
		MemorySizeInBytes = PAGE_SIZE
		manageable_aperture_t *aperture = svm.svm_dgpu_alt_aperture;

		manageable_aperture_ops_t reserved_aperture_ops =
		{
			reserved_aperture_allocate_aligned,
			reserved_aperture_release
		};
	*/
	printf("-----------------------\n");
	printf("1. allocate physical memory on gpu\n");

	void * mem;
	uint64_t handle;
	uint32_t ioc_flag = KFD_IOC_ALLOC_MEM_FLAGS_MMIO_REMAP | KFD_IOC_ALLOC_MEM_FLAGS_WRITABLE | KFD_IOC_ALLOC_MEM_FLAGS_COHERENT;

	mem = aperture_allocate_area(&svm_dgpu_alt_aperture, NULL, sys_page_size, svm_dgpu_alt_aperture.align);
	handle = kfd_allocate_memory(&svm_dgpu_alt_aperture, mem,sys_page_size, ioc_flag);
	vm_obj = fmm_allocate_memory_object(&svm_dgpu_alt_aperture, mem, sys_page_size, handle, ioc_flag);

	return mem;
}

// ------------------------------------------------------------------
#include <errno.h>
void fmm_map_to_cpu(void * address)
{
	printf("-----------------------\n");
	printf("2. map drm_render_fd to cpu\n");
	void * rtn;
	rtn = mmap(address, sys_page_size, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_FIXED, drm_render_fd, mmap_offset);
	fprintf(stderr, "\terror: %d = %s\n", errno, strerror(errno));
	printf("\n");
}

// ------------------------------------------------------------------
int _fmm_map_to_gpu(manageable_aperture_t *app, void *address, uint64_t size, vm_object_t *obj, int *nodes_to_map, uint32_t nodes_array_size)
{
	struct kfd_ioctl_map_memory_to_gpu_args args = { 0 };
	vm_object_t *object;
	int ret = 0;

	object = vm_obj;

	/* For a memory region that is registered by user pointer, changing
	 * mapping nodes is not allowed, so we don't need to check the mapping
	 * nodes or map if it's already mapped. Just increase the reference.
	 */
	if (object->userptr && object->mapping_count)
	{
		++object->mapping_count;
		return ret;
	}

	args.handle = object->handle;
	if (nodes_to_map)
	{
		/* If specified, map the requested */
		args.device_ids_array_ptr = (uint64_t)nodes_to_map;
		args.n_devices = nodes_array_size / sizeof(uint32_t);
	}
	else if (object->registered_device_id_array_size > 0)
	{
		/* otherwise map all registered */
		args.device_ids_array_ptr = (uint64_t)object->registered_device_id_array;
		args.n_devices = object->registered_device_id_array_size / sizeof(uint32_t);
	}
	else
	{
		/* not specified, not registered: map all GPUs */
		printf("\tmap all gpus \n");
		printf("\talloced gpu memory handle = 0x%016lX\n", args.handle);
		args.device_ids_array_ptr = (uint64_t)all_gpu_id_array;
		args.n_devices = all_gpu_id_array_size / sizeof(uint32_t);
	}
	args.n_success = 0;

	ret = kmtIoctl(kfd_fd, AMDKFD_IOC_MAP_MEMORY_TO_GPU, &args);

//	add_device_ids_to_mapped_array(object, (uint32_t *)args.device_ids_array_ptr, args.n_success * sizeof(uint32_t));
//	print_device_id_array((uint32_t *)object->mapped_device_id_array, object->mapped_device_id_array_size);

	object->mapping_count = 1;
	/* Mapping changed and lifecycle of object->mapped_node_id_array
	 * terminates here. Free it and allocate on next query
	 */
	if (object->mapped_node_id_array)
	{
		free(object->mapped_node_id_array);
		object->mapped_node_id_array = NULL;
	}

	return ret;
}
void fmm_map_to_gpu(void *address, uint64_t size, uint64_t *gpuvm_address)
{
	printf("-----------------------\n");
	printf("3. map allocated memory to gpu\n");
	manageable_aperture_t *aperture;
	vm_object_t *object;
	uint32_t i;
	int ret;

	/* Special handling for scratch memory */
	if (address >= scratch_physical.base &&	address <= scratch_physical.limit)
	{
		//return _fmm_map_to_gpu_scratch(&scratch_physical, address, size);
	}

	object = vm_obj;
	aperture = &svm_dgpu_alt_aperture;
	all_gpu_id_array = (uint32_t*)malloc(sizeof(uint32_t) * 1);
	all_gpu_id_array[0] = gGpuId;
	all_gpu_id_array_size = 1;
	all_gpu_id_array_size *= sizeof(uint32_t);
	/* Successful vm_find_object returns with the aperture locked */

	if (aperture == &cpuvm_aperture)
	{
		/* Prefetch memory on APUs with dummy-reads */
		//fmm_check_user_memory(address, size);
		ret = 0;
	}
	else if (object->userptr)
	{
		//ret = _fmm_map_to_gpu_userptr(address, size, gpuvm_address, object);
	}
	else
	{
		ret = _fmm_map_to_gpu(aperture, address, size, object, NULL, 0);
		/* Update alternate GPUVM address only for
		 * CPU-invisible apertures on old APUs
		 */
		if (!ret && gpuvm_address && !aperture->is_cpu_accessible)
			*gpuvm_address = VOID_PTRS_SUB(object->start, aperture->base);
	}

//	pthread_mutex_unlock(&aperture->fmm_mutex);
	printf("\n");
}

// ------------------------------------------------------------------
void map_mmio()
{
	printf("=======================\n");
	printf("map mmio\n");

	mmio_addr = allocate_device();
	fmm_map_to_cpu(mmio_addr);
	fmm_map_to_gpu(mmio_addr, sys_page_size, NULL);
}

// ==================================================================
// ==================================================================
void init_mmio_aperture()
{
	printf("=======================\n");
	printf("init mmio apertures\n");
	mmio_aperture.base = mmio_addr;
	mmio_aperture.limit = (void *)((char *)mmio_aperture.base + sys_page_size - 1);
	printf("\tmmio  base = 0x%016lX.\tlimit = 0x%016lX\n", mmio_aperture.base, mmio_aperture.limit);
}

// ==================================================================
// ==================================================================
void kmt_mem_test()
{
	printf("***********************\n");
	printf("* kmt memory test     *\n");
	printf("***********************\n");

	open_drm();
	acquire_vm_from_drm();

	get_process_apertures();
	init_svm_apertures();

	map_mmio();
	init_mmio_aperture();

	close_drm();	
}
