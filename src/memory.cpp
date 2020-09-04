#include "kmthsa.h"

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

static bool dbg_print = false;

int gDrmFd;

#define MAX_VM_OBJ_NUM	(32)
uint32_t vm_obj_cnt = 0;
vm_object_t * vm_obj_array[MAX_VM_OBJ_NUM];

/*
 * 从get_process_apertures()获得gpuvm_limit，该limit如果大于等于47-bit，则GPU为gfx9及以后
 * 此时GPU的虚拟地址可以覆盖全部CPU的地址范围
 * 或者mmap不会超出GPU的可handle的地址范围
 */
struct kfd_process_device_apertures * kfd_dev_aperture; // 从驱动获得的aperture,在驱动层中定义
manageable_aperture_t svm_dgpu_aperture;// svm_dgpu_alt_aperture

// ==================================================================

void open_drm()
{
	int drm_render_minor = kmtReadKey(KFD_NODE_PROP(gNodeIdx), "drm_render_minor");

	gDrmFd = open(DRM_RENDER_DEVICE(drm_render_minor), O_RDWR | O_CLOEXEC);
	assert(gDrmFd != 0);
	printf("\topen node %d drm: \"%s\". fd = %d\n", gNodeIdx, DRM_RENDER_DEVICE(drm_render_minor), gDrmFd);
}
void close_drm()
{
	close(gDrmFd);
	printf("\tclose drm rander file. fd = %d.\n", gDrmFd);
}

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
	uint32_t cpuMemBankCount, gpuMemBankCount;
	HsaMemoryProperties cpuMemProp, gpuMemProp;

	cpuMemBankCount = kmtReadKey(KFD_NODE(0) + string("/properties"), "mem_banks_count");
	printf("\tget cpu memory topology, bank count = %d\n", cpuMemBankCount);
	for (uint32_t i = 0; i < cpuMemBankCount; i++)
	{
		string bank_file = string(KFD_NODE(0)) + string("/mem_banks/") + std::to_string(i) + string("/properties");
		cpuMemProp.HeapType = (HSA_HEAPTYPE)kmtReadKey(bank_file, "heap_type");
		cpuMemProp.SizeInBytes = kmtReadKey(bank_file, "size_in_bytes");
		cpuMemProp.Flags.MemoryProperty = (uint32_t)kmtReadKey(bank_file, "flags");

		printf("\t\t\t-----------------------\n");
		printf("\t\t\tproperties file = %s\n", bank_file.c_str());
		printf("\t\t\theap type = %d.\n", cpuMemProp.HeapType); // 0 = HSA_HEAPTYPE_SYSTEM
		printf("\t\t\tsize = %.3f(GB).\n", cpuMemProp.SizeInBytes / 1024.0 / 1024.0 / 1024.0);
	}

	gpuMemBankCount = kmtReadKey(KFD_NODE(gNodeIdx) + string("/properties"), "mem_banks_count");
	printf("\tget gpu memory topology, bank count = %d\n", gpuMemBankCount);
	for (uint32_t i = 0; i < cpuMemBankCount; i++)
	{
		string bank_file = string(KFD_NODE(gNodeIdx)) + string("/mem_banks/") + std::to_string(i) + string("/properties");
		gpuMemProp.HeapType = (HSA_HEAPTYPE)kmtReadKey(bank_file, "heap_type");
		gpuMemProp.SizeInBytes = kmtReadKey(bank_file, "size_in_bytes");
		gpuMemProp.Flags.MemoryProperty = (uint32_t)kmtReadKey(bank_file, "flags");

		printf("\t\t\t-----------------------\n");
		printf("\t\t\tproperties file = %s\n", bank_file.c_str());
		printf("\t\t\theap type = %d.\n", gpuMemProp.HeapType); // 2 = HSA_HEAPTYPE_FRAME_BUFFER_PRIVATE
		printf("\t\t\tsize = %.3f(GB).\n", gpuMemProp.SizeInBytes / 1024.0 / 1024.0 / 1024.0);
	}

	uint32_t cacheCount;
	uint32_t dataCacheCnt = 0;
	uint32_t instrCacheCnt = 0;
	cacheCount = kmtReadKey(KFD_NODE(gNodeIdx) + string("/properties"), "caches_count");
	printf("\tget gpu cache topology, cache count = %d\n", cacheCount);
	for (uint32_t i = 0; i < cacheCount; i++)
	{
		HsaCacheType cacheType;
		string cache_path = string(KFD_NODE(gNodeIdx)) + string("/caches/") + std::to_string(i) + string("/properties");
		cacheType.Value = (uint32_t)kmtReadKey(cache_path, "type");
		uint32_t cacheLevel = (uint32_t)kmtReadKey(cache_path, "level");
		uint32_t cacheSize = (uint32_t)kmtReadKey(cache_path, "size");

		string cachetype = "";
		if (cacheType.ui32.Data == 1) { cachetype = cachetype + "Data,"; dataCacheCnt++; }
		if (cacheType.ui32.Instruction == 1) { cachetype = cachetype + "Instruction,"; instrCacheCnt++; }
		if (cacheType.ui32.CPU == 1) { cachetype = cachetype + "CPU,"; }
		if (cacheType.ui32.HSACU == 1) { cachetype = cachetype + "HSACU,"; }

		printf("\t\t[%02d]: level = %d, size = %d, type = %s\n", i, cacheLevel, cacheSize, cachetype.c_str());
	}
	
	printf("\t\tcache count             = %d.\n", cacheCount);
	printf("\t\tdata cache count        = %d.\n", dataCacheCnt);
	printf("\t\tinstruction cache count = %d.\n", instrCacheCnt);
}
void acquire_vm_from_drm()
{
	int rtn;

	struct kfd_ioctl_acquire_vm_args args_vm;
	args_vm.gpu_id = gGpuId;
	args_vm.drm_fd = gDrmFd;
	rtn = kmtIoctl(gKfdFd, AMDKFD_IOC_ACQUIRE_VM, (void *)&args_vm); rtn = 0;
	assert(rtn == 0);

	printf("\tacquire_vm_from_drm\n");
}
void get_process_apertures()
{
	int rtn;

	kfd_dev_aperture = (kfd_process_device_apertures*)calloc(gNodeNum, sizeof(struct kfd_process_device_apertures));

	struct kfd_ioctl_get_process_apertures_new_args args_new = { 0 };
	args_new.kfd_process_device_apertures_ptr = (uintptr_t)kfd_dev_aperture;
	args_new.num_of_nodes = gNodeNum;
	rtn = kmtIoctl(gKfdFd, AMDKFD_IOC_GET_PROCESS_APERTURES_NEW, (void *)&args_new);
	int aperture_num = args_new.num_of_nodes;
	assert(rtn == 0);

	printf("\tget_process_apertures\n");
	printf("\t\tkfd gpu device apertures number = %d.\n", aperture_num);
	for (int i = 0; i < aperture_num; i++)
	{
		printf("\t\t\t-----------------------\n");
		printf("\t\t\tprocess apertures %d; gpu_id = %d; pad = %d.\n", i, kfd_dev_aperture[i].gpu_id, kfd_dev_aperture[i].pad);
		printf("\t\t\tlds     base = 0x%016lX.\tlimit = %ld(TB).\n", kfd_dev_aperture[i].lds_base, kfd_dev_aperture[i].lds_limit / 1024 / 1024 / 1024 / 1024);
		printf("\t\t\tscratch base = 0x%016lX.\tlimit = %ld(TB).\n", kfd_dev_aperture[i].scratch_base, kfd_dev_aperture[i].scratch_limit / 1024 / 1024 / 1024 / 1024);
		printf("\t\t\tgpuvm   base = 0x%016lX.\tlimit = %ld(TB).\n", kfd_dev_aperture[i].gpuvm_base, kfd_dev_aperture[i].gpuvm_limit / 1024 / 1024 / 1024 / 1024);
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

void * mmap_aperture_allocate(uint64_t memSize)
{
	// 在内存的空间找到一个可以map的空间,并将地址返回

	memSize += gPageSize;//aligned_padded_size = size + align + 2*guard_size - PAGE_SIZE;

	void * mem_addr;
	int prot = PROT_NONE;
	int flag = MAP_ANONYMOUS| MAP_PRIVATE | MAP_NORESERVE ;
	mem_addr = mmap(0, memSize, prot, flag, -1, 0);
	assert(mem_addr != MAP_FAILED);

	if (dbg_print)
	{
		printf("\tmmap_aperture_allocate\n");
		printf("\t\tanonymous map\n");
		printf("\t\taddr = 0x%016lX\n", mem_addr);
		printf("\t\tsize = 0x%016lX\n", memSize);
	}

	return mem_addr;
}
void mmap_aperture_release(void * memAddr, uint64_t memSize)
{
	munmap(memAddr, memSize);

	if (dbg_print)
	{
		printf("\tmmap aperture release:\n");
		printf("\t\taddress = 0x%016lX\n", memAddr);
		printf("\t\tsize    = 0x%016lX\n", memSize);
	}
}

void kfd_ioctrl_allocate_memory(void *memAddr, uint64_t memSize, uint32_t iocFlag, uint64_t *mmap_offset, uint64_t *handle)
{
	struct kfd_ioctl_alloc_memory_of_gpu_args args = { 0 };
	args.gpu_id = gGpuId;
	args.va_addr = (uint64_t)memAddr;
	args.size = memSize;
	args.flags = iocFlag | KFD_IOC_ALLOC_MEM_FLAGS_NO_SUBSTITUTE;
	if (iocFlag & KFD_IOC_ALLOC_MEM_FLAGS_USERPTR)
		args.mmap_offset = *mmap_offset;
	args.mmap_offset = (uint64_t)memAddr;

	int rtn = kmtIoctl(gKfdFd, AMDKFD_IOC_ALLOC_MEMORY_OF_GPU, &args);
	assert(rtn == 0);

	if (dbg_print)
	{
		printf("\tkfd ioctrl allocate memory:\n");
		printf("\t\targs.flags  = 0x%08X\n", args.flags);
		printf("\t\tusr  offset = 0x%016lX\n", *mmap_offset);
		printf("\t\targs.vaaddr = 0x%016lX\n", args.va_addr);
		printf("\t\targs.offset = 0x%016lX\n", args.mmap_offset);
		printf("\t\targs.handle = 0x%016lX\n", args.handle);
		printf("\t\tsize        = 0x%016lX\n", memSize);
	}

	*mmap_offset = args.mmap_offset;
	*handle = args.handle;
}
void kfd_ioctrl_free_memory(uint64_t handle)
{
	struct kfd_ioctl_free_memory_of_gpu_args args = { 0 };
	args.handle = handle;
	kmtIoctl(gKfdFd, AMDKFD_IOC_FREE_MEMORY_OF_GPU, &args);

	if (dbg_print)
	{
		printf("\tkfd ioctrl free memory:\n");
		printf("\t\thandle  = 0x%016lX\n", handle);
	}
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

	if (dbg_print)
	{
		printf("\tfind_vm_obj idx = %d\n", obj_idx);
		printf("\t\tvmobj start  = 0x%016lX\n", vm_obj->start);
		printf("\t\tvmobj handle = 0x%016lX\n", vm_obj->handle);
		printf("\t\tvmobj size   = 0x%016lX\n", vm_obj->size);
	}

	return vm_obj;
}
vm_object_t * create_memory_object(void *memAddr, uint64_t memSize, uint64_t handle)
{
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

	vm_obj_array[vm_obj_cnt] = vm_obj;
	vm_obj_cnt++;

	if (dbg_print)
	{
		printf("\tcreate %dth vm object:\n", vm_obj_cnt);
		printf("\t\tvmobj start  = 0x%016lX\n", vm_obj->start);
		printf("\t\tvmobj handle = 0x%016lX\n", vm_obj->handle);
		printf("\t\tvmobj size   = 0x%016lX\n", vm_obj->size);
	}

	return vm_obj;
}
void release_memory_object(vm_object_t * vm_obj)
{
	free(vm_obj);
}

// ==================================================================

void kmtInitMem()
{
	printf("kmtInitMem\n");

	get_mem_topology();

	open_drm();
	acquire_vm_from_drm();
	get_process_apertures();
	init_svm_apertures();

	printf("\n");
}
void kmtDeInitMem()
{
	printf("kmtDeInitMem\n");
	close_drm();
}

void * KmtAllocDoorbell(uint64_t memSize, uint64_t doorbell_offset)
{
	if (dbg_print)
		printf("KmtAllocDoorbell\n");

	uint32_t ioc_flag = 0;
	void * mem_addr = NULL;
	uint64_t mmap_offset = 0;
	uint64_t handle;

	mem_addr = mmap_aperture_allocate(memSize);

	ioc_flag |= KFD_IOC_ALLOC_MEM_FLAGS_DOORBELL;
	ioc_flag |= KFD_IOC_ALLOC_MEM_FLAGS_WRITABLE;
	ioc_flag |= KFD_IOC_ALLOC_MEM_FLAGS_COHERENT;
	kfd_ioctrl_allocate_memory(mem_addr, memSize, ioc_flag, &mmap_offset, &handle);

	create_memory_object(mem_addr, memSize, handle);

	bool HostAccess = true;
	int map_fd = doorbell_offset >= (1ULL << 40) ? gKfdFd : gDrmFd; // if doorbell use kfd_fd
	int prot = HostAccess ? PROT_READ | PROT_WRITE : PROT_NONE;
	int flag = HostAccess ? MAP_SHARED | MAP_FIXED : MAP_PRIVATE | MAP_FIXED;
	void * map_ptr = mmap(mem_addr, memSize, prot, flag, map_fd, doorbell_offset);
	assert(map_ptr != MAP_FAILED);
	if (dbg_print)
	{
		printf("\tmmap to file\n");
		printf("\t\tmap_fd = %d, HostAccess = %d\n", map_fd, HostAccess);
		printf("\t\tmap addr   = 0x%016lX\n", mem_addr);
		printf("\t\tmap offset = 0x%016lX\n", mmap_offset);
		printf("\t\tmap_ptr    = 0x%016lX\n", map_ptr);
	}

	return mem_addr;
}
void * KmtAllocDevice(uint64_t memSize)
{
	if (dbg_print)
		printf("KmtAllocDevice\n");

	uint32_t ioc_flag = 0;
	void * mem_addr = NULL;
	uint64_t mmap_offset = 0;
	uint64_t handle;

	mem_addr = mmap_aperture_allocate(memSize);

	ioc_flag |= KFD_IOC_ALLOC_MEM_FLAGS_VRAM;
	ioc_flag |= KFD_IOC_ALLOC_MEM_FLAGS_EXECUTABLE;
	ioc_flag |= KFD_IOC_ALLOC_MEM_FLAGS_WRITABLE;
	kfd_ioctrl_allocate_memory(mem_addr, memSize, ioc_flag, &mmap_offset, &handle);

	//create_memory_object(mem_addr, memSize, handle); where kmt put it

	bool HostAccess = false;
	int map_fd = mmap_offset >= (1ULL << 40) ? gKfdFd : gDrmFd; // if doorbell use kfd_fd
	int prot = HostAccess ? PROT_READ | PROT_WRITE : PROT_NONE;
	int flag = HostAccess ? MAP_SHARED | MAP_FIXED : MAP_PRIVATE | MAP_FIXED;
	void * map_ptr = mmap(mem_addr, memSize, prot, flag, map_fd, mmap_offset);
	assert(map_ptr != MAP_FAILED);
	if (dbg_print)
	{
		printf("\tmmap to dev file\n");
		printf("\t\tmap_fd = %d, HostAccess = %d\n", map_fd, HostAccess);
		printf("\t\tmap addr   = 0x%016lX\n", mem_addr);
		printf("\t\tmap offset = 0x%016lX\n", mmap_offset);
		printf("\t\tmap_ptr    = 0x%016lX\n", map_ptr);
	}

	create_memory_object(mem_addr, memSize, handle);

	return mem_addr;
}
void * KmtAllocHost(uint64_t memSize, bool isPage)
{
	if (dbg_print)
		printf("KmtAllocHost\n");

	uint32_t ioc_flag = 0;
	void * mem_addr = NULL;
	uint64_t mmap_offset = 0;
	uint64_t handle;

	if (isPage == false)
	{
		mem_addr = mmap_aperture_allocate(memSize);

		ioc_flag |= KFD_IOC_ALLOC_MEM_FLAGS_GTT;
		ioc_flag |= KFD_IOC_ALLOC_MEM_FLAGS_EXECUTABLE;
		ioc_flag |= KFD_IOC_ALLOC_MEM_FLAGS_COHERENT;
		ioc_flag |= KFD_IOC_ALLOC_MEM_FLAGS_WRITABLE;
		kfd_ioctrl_allocate_memory(mem_addr, memSize, ioc_flag, &mmap_offset, &handle);

		int map_fd = mmap_offset >= (1ULL << 40) ? gKfdFd : gDrmFd;
		int prot = PROT_READ | PROT_WRITE;
		int flag = MAP_SHARED | MAP_FIXED;
		void * map_ptr = mmap(mem_addr, memSize, prot, flag, map_fd, mmap_offset);
		assert(map_ptr != MAP_FAILED);
		if (dbg_print)
		{
			printf("\tmmap to file\n");
			printf("\t\tmap_fd = %d, HostAccess = 1\n", map_fd);
			printf("\t\tmap addr   = 0x%016lX\n", mem_addr);
			printf("\t\tmap offset = 0x%016lX\n", mmap_offset);
			printf("\t\tmap_ptr    = 0x%016lX\n", map_ptr);
		}

		create_memory_object(mem_addr, memSize, handle);
	}
	if (isPage == true)
	{
		mem_addr = mmap_aperture_allocate(memSize);

		int prot = PROT_READ | PROT_WRITE;
		int flag = MAP_ANONYMOUS | MAP_PRIVATE | MAP_FIXED;
		void * map_ptr = mmap(mem_addr, memSize, prot, flag, -1, mmap_offset);
		assert(map_ptr != MAP_FAILED);
		if (dbg_print)
		{
			printf("\tmmap anonymous\n");
			printf("\t\tmap addr   = 0x%016lX\n", mem_addr);
			printf("\t\tmap offset = 0x%016lX\n", mmap_offset);
			printf("\t\tmap_ptr    = 0x%016lX\n", map_ptr);
		}

		ioc_flag |= KFD_IOC_ALLOC_MEM_FLAGS_USERPTR;
		ioc_flag |= KFD_IOC_ALLOC_MEM_FLAGS_EXECUTABLE;
		ioc_flag |= KFD_IOC_ALLOC_MEM_FLAGS_COHERENT;
		ioc_flag |= KFD_IOC_ALLOC_MEM_FLAGS_WRITABLE;
		kfd_ioctrl_allocate_memory(mem_addr, memSize, ioc_flag, &mmap_offset, &handle);

		create_memory_object(mem_addr, memSize, handle);
	}

	return mem_addr;
}

void KmtReleaseMemory(void * memAddr)
{
	if (dbg_print)
		printf("KmtReleaseMemory\n");

	vm_object_t * vm_obj = find_vm_obj(memAddr);

	kfd_ioctrl_free_memory(vm_obj->handle);
	mmap_aperture_release(vm_obj->start, vm_obj->size);
	release_memory_object(vm_obj);
}

void KmtMapToGpu(void * memAddr, uint64_t memSize, uint64_t * gpuvm_address)
{
	if (dbg_print)
		printf("KmtMapToGpu\n");

	vm_object_t * vm_obj = find_vm_obj(memAddr);

	if (vm_obj->userptr) // map gpu to user pointer
	{
		uint32_t page_offset = (uint64_t)memAddr & (gPageSize - 1);
		memAddr = vm_obj->start;
		memSize = vm_obj->size;
		vm_obj->mapping_count++;
		if (gpuvm_address != NULL)
		{
			*gpuvm_address = (uint64_t)((char*)memAddr + page_offset);
		}
		if (dbg_print)
		{
			printf("\tmap to userptr\n");
			printf("\t\tmap addr      = 0x%016lX\n", memAddr);
			printf("\t\tgpuvm_address = 0x%016lX\n", gpuvm_address);
		}
		return;
	}

	struct kfd_ioctl_map_memory_to_gpu_args args = { 0 };
	args.handle = vm_obj->handle;
	args.device_ids_array_ptr = (uint64_t)&gGpuId;
	args.n_devices = 1;
	args.n_success = 0;
	int rtn = kmtIoctl(gKfdFd, AMDKFD_IOC_MAP_MEMORY_TO_GPU, &args);
	assert(rtn == 0);
	vm_obj->mapping_count = 1;
	if (dbg_print)
	{
		printf("\tioctrl map to gpu\n");
		printf("\t\targs.handle  = 0x%016lX\n", args.handle);
	}
}
void KmtUnmapFromGpu(void * memAddr)
{
	if (dbg_print)
		printf("KmtUnmapFromGpu\n");

	vm_object_t * vm_obj = find_vm_obj(memAddr);

	struct kfd_ioctl_unmap_memory_from_gpu_args args = { 0 };
	args.handle = vm_obj->handle;
	args.device_ids_array_ptr = (uint64_t)&gGpuId;
	args.n_devices = 1;
	args.n_success = 0;
	int rtn = kmtIoctl(gKfdFd, AMDKFD_IOC_UNMAP_MEMORY_FROM_GPU, &args);
	assert(rtn == 0);
	if (dbg_print)
	{
		printf("\tioctrl map to gpu\n");
		printf("\t\taddress = 0x%016lX\n", memAddr);
	}
}

void * KmtGetVmHandle(void * memAddr)
{
	vm_object_t * vm_obj = find_vm_obj(memAddr);
	if (vm_obj == NULL) return NULL;
	return (void*)vm_obj->handle;
}

// ==================================================================

void * HsaAllocCPU(uint64_t memSize)
{
	printf("HsaAllocCPU\n");

	void * mem_addr = NULL;
	uint64_t mem_size = ALIGN_UP(memSize, gPageSize);

	mem_addr = KmtAllocHost(mem_size);
	KmtMapToGpu(mem_addr, mem_size);
	
	printf("mem_addr = 0x%016lX\n", mem_addr);
	printf("\n");
	return mem_addr;
}
void * HsaAllocGPU(uint64_t memSize)
{
	printf("HsaAllocGPU\n");

	void * mem_addr = NULL;
	uint64_t mem_size = ALIGN_UP(memSize, gPageSize);

	mem_addr = KmtAllocDevice(mem_size);
	KmtMapToGpu(mem_addr, mem_size);
	
	printf("mem_addr = 0x%016lX\n", mem_addr);
	printf("\n");
	return mem_addr;
}
void HsaFreeMem(void * memAddr)
{
	printf("HsaFreeMem\n");
	KmtUnmapFromGpu(memAddr);
	KmtReleaseMemory(memAddr);
}

void RunMemoryTest()
{
	dbg_print = true;
	void *addr;

	printf("\n================ Alloc Device ==============\n");
	addr = HsaAllocGPU(1024);
	HsaFreeMem(addr);

	printf("\n================ Alloc UserPrt =============\n");
	addr = HsaAllocCPU(1024);
	HsaFreeMem(addr);

	printf("\n================ Alloc GTT =================\n"); // EVENT
	addr = KmtAllocHost(1024, false);
	KmtMapToGpu(addr, 1024);

	printf("\n");
}
