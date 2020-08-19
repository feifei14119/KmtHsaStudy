#include "kmt_test.h"

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
	//rbtree_node_t node;
	//rbtree_node_t user_node;

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
	//rbtree_t tree;
	//rbtree_t user_tree;
	//	pthread_mutex_t fmm_mutex;
	bool is_cpu_accessible;
	//	const manageable_aperture_ops_t *ops;
} manageable_aperture_t;

// ==================================================================
// ==================================================================
int gDrmFd;
uint32_t MemBankCount, CacheCount;
HsaMemoryProperties MemBankProp;

#define MAX_VM_OBJ_NUM	(32)
uint32_t vm_obj_cnt = 0;
vm_object_t * vm_obj_array[MAX_VM_OBJ_NUM];

/*
 * 从get_process_apertures()获得gpuvm_limit，该limit如果大于等于47-bit，则GPU为gfx9及以后
 * 此时GPU的虚拟地址可以覆盖全部CPU的地址范围
 * 或者mmap不会超出GPU的可handle的地址范围
 */
int aperture_num;
struct kfd_process_device_apertures * kfd_dev_aperture; // 从驱动获得的aperture,在驱动层中定义
manageable_aperture_t svm_dgpu_aperture;// svm_dgpu_alt_aperture

// ==================================================================
// ==================================================================
void open_drm()
{
	int drm_render_minor = kmtReadKey(KFD_NODE_PROP(gNodeIdx), "drm_render_minor");

	gDrmFd = open(DRM_RENDER_DEVICE(drm_render_minor), O_RDWR | O_CLOEXEC);
	assert(gDrmFd != 0);
	printf("\topen node %d drm render device: \"%s\". fd = %d\n", gNodeIdx, DRM_RENDER_DEVICE(drm_render_minor), gDrmFd);
}
void close_drm()
{
	close(gDrmFd);
	printf("\tclose drm rander file. fd = %d.\n", gDrmFd);
}

HsaMemFlags gpu_mem_flag;
HsaMemFlags cpu_mem_flag;
HsaMemFlags apu_mem_flag;
void get_mem_topology()
{
	/*
	 * CPU 只有一个memory bank, 属性为 HSA_HEAPTYPE_SYSTEM
	 *		fine_grain = true;
	 *		IsSystem() = true;
	 *		IsLocalMemory() = false;
     *		mem_flag.CachePolicy = HSA_CACHING_CACHED;
     *		mem_flag.PageSize = HSA_PAGE_SIZE_4KB;
     *		mem_flag.NoSubstitute = 1;
     *		mem_flag.HostAccess = 1;
     *		mem_flag.NonPaged = 0;
     *		mem_flag.CoarseGrain = 1;
	 *
	 * GPU 只有一个memory bank，属性为 HSA_HEAPTYPE_FRAME_BUFFER_PRIVATE
	 *		fine_grain = false;
	 *		IsLocalMemory() = true;
     *		mem_flag.PageSize = HSA_PAGE_SIZE_4KB;
     *		mem_flag.NoSubstitute = 1;
     *		mem_flag.HostAccess = 0;
     *		mem_flag.NonPaged = 1;
     *		mem_flag.CoarseGrain = 1;
	 */
	printf("\tget memory & caches topology info.\n");
	MemBankCount = kmtReadKey(KFD_NODE(gNodeIdx) + string("/properties"), "mem_banks_count");
	CacheCount = kmtReadKey(KFD_NODE(gNodeIdx) + string("/properties"), "caches_count");

	printf("\t\tmemory bank count = %d.\n", MemBankCount);
	MemBankProp.HeapType = (HSA_HEAPTYPE)kmtReadKey(KFD_NODE(gNodeIdx) + string("/mem_banks/0/properties"), "heap_type");
	MemBankProp.SizeInBytes = kmtReadKey(KFD_NODE(gNodeIdx) + string("/mem_banks/0/properties"), "size_in_bytes");
	MemBankProp.Flags.MemoryProperty = (uint32_t)kmtReadKey(KFD_NODE(gNodeIdx) + string("/mem_banks/0/properties"), "flags");
	MemBankProp.VirtualBaseAddress = 0;
	printf("\t\t\theap type = %d.\n", MemBankProp.HeapType); // 2 = HSA_HEAPTYPE_FRAME_BUFFER_PRIVATE
	printf("\t\t\tsize = %.3f(GB).\n", MemBankProp.SizeInBytes / 1024.0 / 1024.0 / 1024.0);

	printf("\t\tcache count = %d.\n", CacheCount);
	uint32_t cacheCount = 0;
	HsaCacheType cacheType;
	uint32_t cacheLevel;
	uint32_t cacheSize;
	for (uint32_t i = 0; i < CacheCount; i++)
	{
		string cache_path = string(KFD_NODE(gNodeIdx)) + string("/caches/") + std::to_string(i) + string("/properties");
		cacheType.Value = (uint32_t)kmtReadKey(cache_path, "type");
		cacheLevel = (uint32_t)kmtReadKey(cache_path, "level");
		cacheSize = (uint32_t)kmtReadKey(cache_path, "size");
		if (!(cacheType.ui32.HSACU != 1 || cacheType.ui32.Instruction == 1))
		{
			//printf("[%02d] level = %d, size = %d(KB).\n", i, cacheLevel, cacheSize);
			cacheCount++;
		}
	}
	printf("\t\tnon-instruction cache count = %d.\n", cacheCount);
	printf("\t\tinstruction cache count = %d.\n", CacheCount - cacheCount);

	gpu_mem_flag.Value = 0;
	gpu_mem_flag.ui32.NoSubstitute = 1;
	gpu_mem_flag.ui32.NonPaged = 1;
	gpu_mem_flag.ui32.CoarseGrain = 1;
	gpu_mem_flag.ui32.PageSize = HSA_PAGE_SIZE_4KB;

	cpu_mem_flag.Value = 0;
	cpu_mem_flag.ui32.ExecuteAccess = 1;
	cpu_mem_flag.ui32.HostAccess = 1;
	cpu_mem_flag.ui32.NonPaged = 0;
	cpu_mem_flag.ui32.CoarseGrain = 0;
	cpu_mem_flag.ui32.PageSize = HSA_PAGE_SIZE_4KB;

	apu_mem_flag.Value = 0;
	apu_mem_flag.ui32.NoSubstitute = 1;
	apu_mem_flag.ui32.HostAccess = 1;
	apu_mem_flag.ui32.NonPaged = 0;
	apu_mem_flag.ui32.CoarseGrain = 1;
	apu_mem_flag.ui32.PageSize = HSA_PAGE_SIZE_4KB;
}
void acquire_vm_from_drm()
{
	int rtn;

	struct kfd_ioctl_acquire_vm_args args_vm;
	args_vm.gpu_id = gGpuId;
	args_vm.drm_fd = gDrmFd;
	rtn = kmtIoctl(gKfdFd, AMDKFD_IOC_ACQUIRE_VM, (void *)&args_vm); rtn = 0;
	assert(rtn == 0);
}
void get_process_apertures()
{
	int rtn;

	kfd_dev_aperture = (kfd_process_device_apertures*)calloc(gNodeNum, sizeof(struct kfd_process_device_apertures));

	struct kfd_ioctl_get_process_apertures_new_args args_new = { 0 };
	args_new.kfd_process_device_apertures_ptr = (uintptr_t)kfd_dev_aperture;
	args_new.num_of_nodes = gNodeNum;
	rtn = kmtIoctl(gKfdFd, AMDKFD_IOC_GET_PROCESS_APERTURES_NEW, (void *)&args_new);
	aperture_num = args_new.num_of_nodes;
	assert(rtn == 0);

	printf("\tkfd gpu device apertures number = %d.\n", aperture_num);
	for (int i = 0; i < aperture_num; i++)
	{
		printf("\t\t-----------------------\n");
		printf("\t\tprocess apertures %d; gpu_id = %d; pad = %d.\n", i, kfd_dev_aperture[i].gpu_id, kfd_dev_aperture[i].pad);
		printf("\t\tlds     base = 0x%016lX.\tlimit = %ld(TB).\n", kfd_dev_aperture[i].lds_base, kfd_dev_aperture[i].lds_limit / 1024 / 1024 / 1024 / 1024);
		printf("\t\tscratch base = 0x%016lX.\tlimit = %ld(TB).\n", kfd_dev_aperture[i].scratch_base, kfd_dev_aperture[i].scratch_limit / 1024 / 1024 / 1024 / 1024);
		printf("\t\tgpuvm   base = 0x%016lX.\tlimit = %ld(TB).\n", kfd_dev_aperture[i].gpuvm_base, kfd_dev_aperture[i].gpuvm_limit / 1024 / 1024 / 1024 / 1024);
	}
}
void init_svm_apertures()
{
	svm_dgpu_aperture.base = (void *)kfd_dev_aperture->gpuvm_base;
	svm_dgpu_aperture.limit = (void *)kfd_dev_aperture->gpuvm_limit;
	svm_dgpu_aperture.align = gPageSize;
	svm_dgpu_aperture.guard_pages = 1;
	svm_dgpu_aperture.is_cpu_accessible = true;

	printf("\tinit svm dgpu aperture.\n");
	printf("\t\tsvm gpu base = 0x%016lX.\tlimit = %ld(TB).\n", kfd_dev_aperture->gpuvm_base, kfd_dev_aperture->gpuvm_limit / 1024 / 1024 / 1024 / 1024);
}

vm_object_t * find_vm_obj(void * memAddr)
{
	uint32_t obj_idx = 0;
	for (obj_idx = 0; obj_idx < MAX_VM_OBJ_NUM; obj_idx++)
	{
		if (vm_obj_array[obj_idx] == NULL)
			continue;

		if (vm_obj_array[obj_idx]->start == (void*)memAddr)
			break;
	}

	vm_object_t * vm_obj = (obj_idx == MAX_VM_OBJ_NUM) ? NULL : vm_obj_array[obj_idx];

	return vm_obj;
}
void * find_vm_handle(void * memAddr)
{
	vm_object_t * vm_obj = find_vm_obj(memAddr);
	if (vm_obj == NULL) return NULL;
	return (void*)vm_obj->handle;
}

void * mmap_aperture_allocate(uint64_t memSize)
{
	// 在内存的空间找到一个可以map的空间,并将地址返回
	//printf("\tmmap aperture allocate:\n");

	memSize += gPageSize;//aligned_padded_size = size + align + 2*guard_size - PAGE_SIZE;

	void * mem_addr;
	mem_addr = mmap(0, memSize, PROT_NONE, MAP_ANONYMOUS | MAP_NORESERVE | MAP_PRIVATE, -1, 0);
	assert(mem_addr != MAP_FAILED);

	//printf("\t\talloc addr = 0x%016lX\n", mem_addr);

	return mem_addr;
}
void mmap_aperture_release(void * memAddr, uint64_t memSize)
{
	munmap(memAddr, memSize);

	printf("\tmmap aperture release:\n");
	printf("\t\taddress = 0x%016lX\n", memAddr);
	printf("\t\tsize    = 0x%016lX\n", memSize);
}

void kfd_ioctrl_allocate_memory(void *memAddr, uint64_t memSize, uint32_t iocFlag, uint64_t *mmap_offset, uint64_t *handle)
{
	//printf("\tkfd ioctrl allocate memory:\n");

	struct kfd_ioctl_alloc_memory_of_gpu_args args = { 0 };
	args.gpu_id = gGpuId;
	args.va_addr = (uint64_t)memAddr;
	args.size = memSize;
	args.flags = iocFlag | KFD_IOC_ALLOC_MEM_FLAGS_NO_SUBSTITUTE;
	if (iocFlag & KFD_IOC_ALLOC_MEM_FLAGS_USERPTR)
		args.mmap_offset = *mmap_offset;
	args.mmap_offset = (uint64_t)memAddr;

	//printf("ioc_flags = 0x%08X\n", iocFlag);
	//printf("args.gpu_id = %d\n", args.gpu_id);
	//printf("args.flags = 0x%08X\n", args.flags);
	//printf("args.handle = 0x%016X\n", args.handle);
	//printf("args.mmap_offset = 0x%016X\n", args.mmap_offset);
	int rtn = kmtIoctl(gKfdFd, AMDKFD_IOC_ALLOC_MEMORY_OF_GPU, &args);
	assert(rtn == 0);

	//printf("\t\tusr offset = 0x%016lX\n", *mmap_offset);
	//printf("\t\tmap offset = 0x%016lX\n", args.mmap_offset);
	//printf("\t\tbo  handle = 0x%016lX\n", args.handle);

	*mmap_offset = args.mmap_offset;
	*handle = args.handle;
	//printf("args.handle = 0x%016X\n", args.handle);
	//printf("args.mmap_offset = 0x%016X\n", args.mmap_offset);
}
void kfd_ioctrl_free_memory(uint64_t handle)
{
	struct kfd_ioctl_free_memory_of_gpu_args args = { 0 };
	args.handle = handle;
	kmtIoctl(gKfdFd, AMDKFD_IOC_FREE_MEMORY_OF_GPU, &args);

	printf("\tkfd ioctrl free memory:\n");
	printf("\t\thandle  = 0x%016lX\n", handle);
}

vm_object_t * create_memory_object(void *memAddr, uint64_t memSize, uint64_t handle)
{
	//printf("\tcreate %d vm object:\n", vm_obj_cnt);

	vm_object_t * vm_obj = (vm_object_t *)malloc(sizeof(vm_object_t));
	vm_obj->start = memAddr;
	vm_obj->userptr = NULL;
	vm_obj->userptr_size = 0;
	vm_obj->size = memSize;
	vm_obj->handle = handle;
	vm_obj->registered_device_id_array_size = 0;
	vm_obj->mapped_device_id_array_size = 0;
	vm_obj->registered_device_id_array = NULL;
	vm_obj->mapped_device_id_array = NULL;
	vm_obj->registered_node_id_array = NULL;
	vm_obj->mapped_node_id_array = NULL;
	vm_obj->registration_count = 0;
	vm_obj->mapping_count = 0;
	vm_obj->flags = 0;
	vm_obj->metadata = NULL;
	vm_obj->user_data = NULL;
	vm_obj->is_imported_kfd_bo = false;
	//vm_obj->node.key = rbtree_key((unsigned long)start, size);
	//vm_obj->user_node.key = rbtree_key(0, 0);

	vm_obj_array[vm_obj_cnt] = vm_obj;
	vm_obj_cnt++;

	//printf("\t\tvmobj start  = 0x%016lX\n", vm_obj->start);
	//printf("\t\tvmobj handle = 0x%016lX\n", vm_obj->handle);

	return vm_obj;
}
void release_memory_object(vm_object_t * vm_obj)
{
	free(vm_obj);
}

// ------------------------------------------------------------------
// ------------------------------------------------------------------
void * fmm_allocate_device(uint64_t memSize, uint32_t iocFlag, uint64_t * mmap_offset)
{
	void * mem_addr;
	uint64_t handle;
	uint32_t ioc_flag = iocFlag;
	uint64_t mem_size = memSize;

	mem_addr = mmap_aperture_allocate(mem_size);
	kfd_ioctrl_allocate_memory(mem_addr, mem_size, ioc_flag, mmap_offset, &handle);
	create_memory_object(mem_addr, mem_size, handle);

	return mem_addr;
}
void * fmm_allocate_device_for_host(uint64_t memSize, uint32_t iocFlag, uint64_t * mmap_offset)
{
	void * mem_addr;
	uint64_t handle;
	uint32_t ioc_flag = iocFlag;
	uint64_t mem_size = memSize;

	mem_addr = mmap_aperture_allocate(mem_size);

	void * rtn = mmap(mem_addr, mem_size,
		PROT_READ | PROT_WRITE,
		MAP_ANONYMOUS | MAP_PRIVATE | MAP_FIXED, -1, 0);
	if (rtn == MAP_FAILED) printf("mmap rtn faild.\n");

	kfd_ioctrl_allocate_memory(mem_addr, mem_size, ioc_flag, mmap_offset, &handle);
	create_memory_object(mem_addr, mem_size, handle);

	return mem_addr;
}
void fmm_release_device(void * memAddr)
{
	vm_object_t * vm_obj = find_vm_obj(memAddr);

	kfd_ioctrl_free_memory(vm_obj->handle);
	mmap_aperture_release(vm_obj->start, vm_obj->size);
	release_memory_object(vm_obj);
}
void * fmm_map_to_cpu(void * memAddr, uint64_t memSize, uint64_t mmap_offset, bool HostAccess = true)
{
	//printf("\tmap to cpu: \n");

	int map_fd = mmap_offset >= (1ULL << 40) ? gKfdFd : gDrmFd; // if doorbell use kfd_fd
	int prot = HostAccess ? PROT_READ | PROT_WRITE : PROT_NONE;
	int flag = HostAccess ? MAP_SHARED | MAP_FIXED : MAP_PRIVATE | MAP_FIXED;
	void * map_ptr = mmap(memAddr, memSize, prot, flag, map_fd, mmap_offset);
	assert(map_ptr != MAP_FAILED);

	//printf("\t\tmap file    = %s\n", (map_fd == gKfdFd) ? "kfd_fd" : "drm_fd");
	//printf("\t\tmap offset  = 0x%016lX\n", mmap_offset);
	//printf("\t\treturn addr = 0x%016lX\n", map_ptr);
	return map_ptr;
}
void fmm_map_to_gpu(void * memAddr, uint64_t memSize, uint64_t * gpuvm_address = NULL)
{
	//printf("\tmap to gpu: \n");

	vm_object_t * vm_obj = find_vm_obj(memAddr);
		
	if (vm_obj->userptr) // map gpu to user pointer
	{
		uint32_t page_offset = (uint64_t)memAddr & (gPageSize - 1);
		memAddr = vm_obj->start;
		memSize = vm_obj->size;
		vm_obj->mapping_count++;
		if (gpuvm_address != NULL)
			*gpuvm_address = (uint64_t)((char*)memAddr + page_offset);
		return;
	}
	struct kfd_ioctl_map_memory_to_gpu_args args = { 0 };
	args.handle = vm_obj->handle;
	args.device_ids_array_ptr = (uint64_t)&gGpuId;
	args.n_devices = 1;
	args.n_success = 0;
	//printf("args.handle = 0x%016lX \n", args.handle);
	//printf("args.device_ids_array_ptr = 0x%016lX \n", args.device_ids_array_ptr);
	//printf("args.n_devices = %d \n", args.n_devices);
	//printf("args.n_success = %d \n", args.n_success);
	int rtn = kmtIoctl(gKfdFd, AMDKFD_IOC_MAP_MEMORY_TO_GPU, &args);
	//printf("args.n_success = %d \n", args.n_success);
	assert(rtn == 0);
	vm_obj->mapping_count = 1;

	//printf("\t\tmap addr    = 0x%016lX\n", memAddr);
	//printf("\t\tmap handle  = 0x%016lX\n", vm_obj->handle);
}
void fmm_unmap_from_gpu(void * memAddr)
{
	vm_object_t * vm_obj = find_vm_obj(memAddr);

	struct kfd_ioctl_unmap_memory_from_gpu_args args = { 0 };
	args.handle = vm_obj->handle;
	args.device_ids_array_ptr = (uint64_t)&gGpuId;
	args.n_devices = 1;
	args.n_success = 0;
	int rtn = kmtIoctl(gKfdFd, AMDKFD_IOC_UNMAP_MEMORY_FROM_GPU, &args);
	assert(rtn == 0);

	printf("\tunmap to gpu: \n");
	printf("\t\taddress = 0x%016lX\n", memAddr);
}
void fmm_register_user_memory(void * memAddr, uint64_t memSize)
{
	vm_object_t * vm_obj = find_vm_obj(memAddr);

	if (vm_obj == NULL)
	{
		vm_obj = (vm_object_t*)AllocMemoryUserptr(memAddr, memSize);
	}
	else
	{
		vm_obj->registration_count++;
	}
}

// ==================================================================
// ==================================================================
void MemInit()
{
	printf("\n==============================================\n");
	printf("init mem\n");

	get_mem_topology();

	open_drm();
	acquire_vm_from_drm();
	get_process_apertures();
	init_svm_apertures();
	printf("==============================================\n");
	printf("\n");
}
void MemDeInit()
{
	printf("\n==============================================\n");
	printf("de-init mem\n");

	close_drm();
}
void * AllocMemoryAPU(uint64_t memSize)
{
	/*
	 * 使用rocr分配host系统内存
	 * Agent 为 APU Agent; memory region 为 apu 的 memory
	 */
	printf("\n=======================\n");
	printf("alloc apu memory.\n");

	void * mem_addr = NULL;
	HsaMemFlags mem_flag = apu_mem_flag;
	int mmap_prot = PROT_READ;
	uint64_t mem_size = ALIGN_UP(memSize, gPageSize);

	if (mem_flag.ui32.ExecuteAccess)	mmap_prot |= PROT_EXEC;
	if (!mem_flag.ui32.ReadOnly)		mmap_prot |= PROT_WRITE;

	mem_addr = mmap(NULL, mem_size, mmap_prot, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
	assert(mem_addr != MAP_FAILED);
	printf("\tmmap to cpu aperture:\n");
	printf("\t\taddress = 0x%016lX\n", mem_addr);
	printf("\t\tsize    = 0x%016lX\n", mem_size);

	create_memory_object(mem_addr, mem_size, 0);

	printf("cpu address = 0x%016lX.\n", mem_addr);
	return mem_addr;
}
void * HsaAllocCPU(uint64_t memSize)
{
	/*
	 * 使用rocr分配host系统内存
	 * Agent 为 CPU Agent; memory region 为 cpu 的 memory
	 * map_to_gpu() 将地址 map 到 scratch memory
	 */
	//printf("\n=======================\n");
	//printf("alloc host memory.\n");

	HsaMemFlags mem_flag = cpu_mem_flag;
	uint32_t ioc_flag = 0;
	void * mem_addr = NULL;
	uint64_t mmap_offset = 0;
	uint64_t mem_size = ALIGN_UP(memSize, gPageSize);
	
	if (mem_flag.ui32.AQLQueueMemory)
		mem_size *= 2;

	ioc_flag |= KFD_IOC_ALLOC_MEM_FLAGS_USERPTR;
	ioc_flag |= KFD_IOC_ALLOC_MEM_FLAGS_EXECUTABLE;
	ioc_flag |= KFD_IOC_ALLOC_MEM_FLAGS_COHERENT;
	if (mem_flag.ui32.AQLQueueMemory)	ioc_flag |= KFD_IOC_ALLOC_MEM_FLAGS_AQL_QUEUE_MEM;
	if (!mem_flag.ui32.ReadOnly)		ioc_flag |= KFD_IOC_ALLOC_MEM_FLAGS_WRITABLE;
	

	mem_addr = fmm_allocate_device_for_host(mem_size, ioc_flag, &mmap_offset);

	if (mem_flag.ui32.HostAccess)
	{
		//void * map_ptr = fmm_map_to_cpu(mem_addr, mem_size, mmap_offset);
		fmm_map_to_gpu(mem_addr, mem_size);
		//printf("map  address = 0x%016lX.\n", map_ptr);

		/*if (mem_flag.ui32.AQLQueueMemory)
		{
			uint64_t buf_size = ALIGN_UP(mem_size, gPageSize) / 2;
			memset(map_ptr, 0, mem_size);
			fmm_map_to_cpu((void*)((char*)mem_addr + buf_size), mem_size, mmap_offset);
		}*/
	}

	//printf("host address = 0x%016lX.\n", mem_addr);
	//printf("host offset  = 0x%016lX.\n", mmap_offset);
	//printf("host size    = %.3f(KB).\n", mem_size / 1024.0);
	//printf("=======================\n");

	return mem_addr;
}
void * AllocMemoryGPUVM(uint64_t memSize, bool isHostAccess)
{
	//printf("\n=======================\n");
	//printf("alloc device memory.\n");

	HsaMemFlags mem_flag = gpu_mem_flag;
	uint32_t ioc_flag = 0;
	void * mem_addr = NULL;
	uint64_t mmap_offset = 0;
	uint64_t mem_size = ALIGN_UP(memSize, gPageSize);
	
	if (isHostAccess)
		mem_flag.ui32.HostAccess = true;

	if (mem_flag.ui32.AQLQueueMemory)
		mem_size *= 2;

	ioc_flag |= KFD_IOC_ALLOC_MEM_FLAGS_VRAM;		//
	ioc_flag |= KFD_IOC_ALLOC_MEM_FLAGS_EXECUTABLE;	//
	if (mem_flag.ui32.HostAccess)		ioc_flag |= KFD_IOC_ALLOC_MEM_FLAGS_PUBLIC;
	if (mem_flag.ui32.AQLQueueMemory)	ioc_flag |= KFD_IOC_ALLOC_MEM_FLAGS_AQL_QUEUE_MEM;
	if (!mem_flag.ui32.ReadOnly)		ioc_flag |= KFD_IOC_ALLOC_MEM_FLAGS_WRITABLE;//
	//if (!mem_flag.ui32.CoarseGrain)		ioc_flag |= KFD_IOC_ALLOC_MEM_FLAGS_COHERENT;//

	mem_addr = fmm_allocate_device(mem_size, ioc_flag, &mmap_offset);

	// map host memory
	//if (mem_flag.ui32.HostAccess) // not valid for normal gpu mem allocate
	{
		fmm_map_to_cpu(mem_addr, mem_size, mmap_offset, false);
	}

	// map to gpu
	fmm_map_to_gpu(mem_addr, mem_size);

	//printf("device address = 0x%016lX.\n", mem_addr);
	//printf("device offset  = 0x%016lX.\n", mmap_offset);
	//printf("device size    = %.3f(KB).\n", mem_size / 1024.0);
	//printf("=======================\n");

	return mem_addr;
}
void * AllocMemoryMMIO()
{
	printf("\n=======================\n");
	printf("alloc mmio memory.\n");

	HsaMemFlags mem_flag;
	uint32_t ioc_flag = 0;
	void * mem_addr = NULL;
	uint64_t mmap_offset = 0;
	uint64_t mem_size = gPageSize;

	mem_flag.ui32.NonPaged = 1;
	mem_flag.ui32.HostAccess = 1;

	ioc_flag |= KFD_IOC_ALLOC_MEM_FLAGS_MMIO_REMAP;
	ioc_flag |= KFD_IOC_ALLOC_MEM_FLAGS_WRITABLE;
	ioc_flag |= KFD_IOC_ALLOC_MEM_FLAGS_COHERENT;

	mem_addr = fmm_allocate_device(mem_size, ioc_flag, &mmap_offset);

	// map to cpu
	fmm_map_to_cpu(mem_addr, mem_size, mmap_offset);

	// map to gpu
	fmm_map_to_gpu(mem_addr, mem_size);

	printf("mmio address = 0x%016lX.\n", mem_addr);
	printf("mmio offset  = 0x%016lX.\n", mmap_offset);
	printf("mmio size    = %.3f(KB).\n", mem_size / 1024.0);
	printf("=======================\n");

	return mem_addr;
}
void * AllocMemoryDoorbell(uint64_t memSize, uint64_t doorbell_offset)
{
	//printf("\n=======================\n");
	//printf("allocate doorbell memory.\n");

	HsaMemFlags mem_flag;
	uint32_t ioc_flag = 0;
	void * mem_addr = NULL;
	uint64_t mmap_offset = 0;
	uint64_t mem_size = ALIGN_UP(memSize, gPageSize);

	ioc_flag |= KFD_IOC_ALLOC_MEM_FLAGS_DOORBELL;
	ioc_flag |= KFD_IOC_ALLOC_MEM_FLAGS_WRITABLE;
	ioc_flag |= KFD_IOC_ALLOC_MEM_FLAGS_COHERENT;

	mem_addr = fmm_allocate_device(mem_size, ioc_flag, &mmap_offset);

	// map to host memory
	void *ret = fmm_map_to_cpu(mem_addr, mem_size, doorbell_offset);
	assert(ret != MAP_FAILED);

	// map for GPU access
	fmm_map_to_gpu(mem_addr, mem_size);

	//printf("doorbell address = 0x%016lX.\n", mem_addr);
	//printf("doorbell offset  = 0x%016lX.\n", doorbell_offset);
	//printf("doorbell size    = %.3f(KB).\n", mem_size / 1024.0);
	//printf("=======================\n");

	return mem_addr;
}
void * AllocMemoryUserptr(void * memAddr, uint64_t memSize)
{
	uint32_t page_offset = (uint64_t)memAddr & (gPageSize - 1);
	uint64_t aligned_addr = (uint64_t)memAddr - page_offset;
	uint64_t aligned_size = ALIGN_UP(page_offset + memSize, gPageSize);
	uint32_t ioc_flag = 0;
	void * mem_addr = NULL;
	uint64_t mmap_offset = 0;
	uint64_t handle;
	uint64_t mem_size = memSize;

	ioc_flag |= KFD_IOC_ALLOC_MEM_FLAGS_USERPTR;
	ioc_flag |= KFD_IOC_ALLOC_MEM_FLAGS_WRITABLE;
	ioc_flag |= KFD_IOC_ALLOC_MEM_FLAGS_EXECUTABLE;
	ioc_flag |= KFD_IOC_ALLOC_MEM_FLAGS_COHERENT;

	mem_addr = fmm_allocate_device(mem_size, ioc_flag, &mmap_offset);

	vm_object_t * vm_obj = find_vm_obj(mem_addr);
	vm_obj->userptr = memAddr;
	vm_obj->userptr_size = mem_size;
	vm_obj->registration_count = 1;

	return vm_obj;
}


void FreeMemoryGPUVM(void * memAddr)
{
	printf("\n=======================\n");
	printf("free device memory.\n");

	fmm_unmap_from_gpu(memAddr);
	fmm_release_device(memAddr);

	printf("free address = 0x%016lX.\n", memAddr);
	printf("=======================\n");
}
void FreeMemoryMMIO(void * memAddr)
{
	printf("\n=======================\n");
	printf("free mmio memory.\n");

	fmm_unmap_from_gpu(memAddr);
	munmap(memAddr, gPageSize);
	fmm_release_device(memAddr);

	printf("free address = 0x%016lX.\n", memAddr);
	printf("=======================\n");
}

// ==================================================================
// ==================================================================
void MemTest()
{
	printf("***********************\n");
	printf("* kmt memory test     *\n");
	printf("***********************\n");
	
	MemInit();


	int32_t len = 128;
	float tmp_data = 0;
	void * mem_addr;

	/*printf("\n***********************\n");
	printf("* mmio memory test    *\n");
	printf("***********************\n");
	mem_addr = AllocMemoryMMIO();
	*reinterpret_cast<uint64_t*>(mem_addr) = 14119;
	uint64_t dev_id = *reinterpret_cast<uint64_t*>(mem_addr);
	printf("device id = 0x%016lX \n", dev_id);
	FreeMemoryMMIO(mem_addr);*/


	printf("\n***********************\n");
	printf("* zero copy device memory test\n");
	printf("***********************\n");
	mem_addr = AllocMemoryGPUVM(len * sizeof(float), true);
	*reinterpret_cast<float*>(mem_addr) = 1.414;
	tmp_data = *reinterpret_cast<float*>(mem_addr);
	printf("zero copy data = %.3f\n", tmp_data);
	FreeMemoryGPUVM(mem_addr);


	/*printf("\n***********************\n");
	printf("* gpu manage cpu memory test\n");
	printf("***********************\n");
	mem_addr = HsaAllocCPU(len * sizeof(float));
	*reinterpret_cast<float*>(mem_addr) = 3.14;
	tmp_data = *reinterpret_cast<float*>(mem_addr);
	printf("cpu data = %.3f\n", tmp_data);


	printf("\n***********************\n");
	printf("* device memory test  *\n");
	printf("***********************\n");
	mem_addr = (float*)AllocMemoryGPUVM(len * sizeof(float));
	FreeMemoryGPUVM(mem_addr);*/
	

	printf("\n");
	MemDeInit();
}
