#include "kmt.h"

uint32_t vega20_eop_buff_size = 4096;
uint32_t vega20_doorbell_size = 8;
uint32_t vega20_cu_num = 60;
uint32_t wave_per_cu = 32;
uint32_t max_wave_per_simd = 10; // for vega20
uint32_t wg_ctx_data_size_per_cu = 344576;
//static uint32_t priority_map[] = { 0, 3, 5, 7, 9, 11, 15 };

// ==================================================================

typedef struct process_doorbell_s
{
	bool use_gpuvm;
	uint32_t size;
	void *mapping;
} process_doorbell_t;
typedef struct kmt_queue_s
{
	uint32_t queue_id;
	uint64_t wptr;
	uint64_t rptr;
	void * eop_buffer;
	void * ctx_save_restore;
	uint32_t ctx_save_restore_size;
	uint32_t ctl_stack_size;
	const void *dev_info;
	bool use_ats;
	uint32_t cu_mask_count;
	uint32_t cu_mask[0];
} kmt_queue_t;

// ==================================================================

void * allocate_doorbell(uint64_t memSize, uint64_t doorbell_offset)
{
	void * mem_addr = NULL;
	uint64_t mem_size = ALIGN_UP(memSize, gPageSize);

	mem_addr = KmtAllocDoorbell(mem_size, doorbell_offset);
	KmtMapToGpu(mem_addr, mem_size);

	return mem_addr;
}
void * allocate_cpu(uint64_t memSize) // same as HsaAllocCpu()
{
	void * mem_addr = NULL;
	uint64_t mem_size = ALIGN_UP(memSize, gPageSize);

	mem_addr = KmtAllocHost(mem_size);
	KmtMapToGpu(mem_addr, mem_size);

	return mem_addr;
}
void * allocate_gpu(uint64_t memSize) // same as HsaAllocGpu()
{
	void * mem_addr = NULL;
	uint64_t mem_size = ALIGN_UP(memSize, gPageSize);

	mem_addr = KmtAllocDevice(mem_size);
	KmtMapToGpu(mem_addr, mem_size);

	return mem_addr;
}

// ==================================================================
void KmtCreateQueue(uint32_t queue_type, void * ring_buff, uint32_t ring_size, HsaQueueResource * queuer_rsc)
{
	HsaMemFlags mem_flag;

	// malloc kmt_queue_t ----------------------------------------------
	kmt_queue_t * kmt_queue = (kmt_queue_t*)allocate_cpu(sizeof(kmt_queue_t));
	memset(kmt_queue, 0, sizeof(kmt_queue_t));
	if (queue_type != KFD_IOC_QUEUE_TYPE_COMPUTE_AQL)
	{
		queuer_rsc->QueueRptrValue = (uintptr_t)&kmt_queue->rptr;
		queuer_rsc->QueueWptrValue = (uintptr_t)&kmt_queue->wptr;
	}
	printf("\n\t-----------------------\n");
	printf("\tallocate kmt queue:\n");
	printf("\t\tkmt queue size = %d(Byte).\n", sizeof(kmt_queue_t));

	// malloc eop buffer ----------------------------------------------
	if (queue_type != KFD_IOC_QUEUE_TYPE_SDMA)
	{
		kmt_queue->eop_buffer = allocate_gpu(vega20_eop_buff_size);
		printf("\n\t-----------------------\n");
		printf("\tallocate eop buffer.\n");
		printf("\t\teop buffer = 0x%016lX.\n", kmt_queue->eop_buffer);
		printf("\t\teop size = %d.\n", vega20_eop_buff_size);

		// malloc ctx switch ----------------------------------------------
		uint32_t ctl_stack_size = vega20_cu_num * wave_per_cu * 8 + 8;
		uint32_t wg_data_size = vega20_cu_num * wg_ctx_data_size_per_cu;
		kmt_queue->ctl_stack_size = ALIGN_UP(ctl_stack_size + 16, gPageSize); // 16 = sizeof(HsaUserContextSaveAreaHeader)
		kmt_queue->ctx_save_restore_size = kmt_queue->ctl_stack_size + ALIGN_UP(wg_data_size, gPageSize);
		kmt_queue->ctx_save_restore = allocate_cpu(kmt_queue->ctx_save_restore_size);
		printf("\n\t-----------------------\n");
		printf("\tallocate context switch.\n");
		printf("\t\tctl stack  size = %.3f(KB).\n", kmt_queue->ctl_stack_size / 1024.0);
		printf("\t\tctx switch size = %.3f(MB).\n", kmt_queue->ctx_save_restore_size / 1024.0 / 1024.0);
		printf("\t\tctx switch addr = %016lX.\n", (uint64_t)kmt_queue->ctx_save_restore);
	}

	// ioctrl create kmt_queue_t ----------------------------------------------
	struct kfd_ioctl_create_queue_args args = { 0 };

	args.gpu_id = gGpuId;
	args.queue_priority = 7; // HSA_QUEUE_PRIORITY_NORMAL
	args.queue_percentage = 100;
	args.queue_type = queue_type;
	args.ring_size = ring_size;
	args.eop_buffer_address = (uintptr_t)kmt_queue->eop_buffer;
	args.eop_buffer_size = vega20_eop_buff_size;
	args.ctl_stack_size = kmt_queue->ctl_stack_size;
	args.ctx_save_restore_size = kmt_queue->ctx_save_restore_size;
	args.ctx_save_restore_address = (uintptr_t)kmt_queue->ctx_save_restore;	
	args.read_pointer_address = queuer_rsc->QueueRptrValue;
	args.write_pointer_address = queuer_rsc->QueueWptrValue;
	args.ring_base_address = (uintptr_t)ring_buff;

	printf("\n\t-----------------------\n");
	printf("\tkmtIoctl AMDKFD_IOC_CREATE_QUEUE\n");
	printf("\t\targs.gpu_id = %d \n", args.gpu_id);
	printf("\t\targs.queue_type = %d \n", args.queue_type);
	printf("\t\targs.queue_percentage = %d \n", args.queue_percentage);
	printf("\t\targs.queue_priority = %d \n", args.queue_priority);
	printf("\t\targs.read_pointer_address  = 0x%016lX \n", args.read_pointer_address);
	printf("\t\targs.write_pointer_address = 0x%016lX \n", args.write_pointer_address);
	printf("\t\targs.ring_base_address     = 0x%016lX \n", args.ring_base_address);
	printf("\t\targs.ring_size = %d \n", args.ring_size);
	int err = kmtIoctl(gKfdFd, AMDKFD_IOC_CREATE_QUEUE, &args);
	assert(err == 0);
	kmt_queue->queue_id = args.queue_id;
	printf("\t-----------------------\n");
	printf("\t\tqueue id = %d.\n", args.queue_id);
	printf("\t\tread  pointer   address = 0x%016lX.\n", queuer_rsc->QueueRptrValue);
	printf("\t\twrite pointer   address = 0x%016lX.\n", queuer_rsc->QueueWptrValue);
	printf("\t\tdoorbell offset address = 0x%016lX.\n", args.doorbell_offset);

	// map doorbell ----------------------------------------------
	process_doorbell_t doorbell;

	doorbell.use_gpuvm = true;
	doorbell.size = vega20_doorbell_size * 1024;
	// map 单位只能是page_size。所以
	uint64_t doorbell_mmap_offset = args.doorbell_offset & ~(uint64_t)(doorbell.size - 1);  // above doorbell size
	uint32_t doorbell_offset = args.doorbell_offset & (doorbell.size - 1);					// inner doorbell size

	printf("\n\t-----------------------\n");
	printf("\tAllocMemoryDoorbell\n");
	printf("\t\tdoorbell mmap_offset = 0x%016lX.\n", doorbell_mmap_offset);
	printf("\t\tdoorbell.size = %d.\n", doorbell.size);
	printf("\t\tdoorbell.use_gpuvm = %d.\n", doorbell.use_gpuvm);
	void * doorbell_addr = allocate_doorbell(doorbell.size, doorbell_mmap_offset);
	doorbell.mapping = doorbell_addr;
	printf("\t-----------------------\n");
	printf("\t\tdoorbell.mapping = 0x%016lX.\n", doorbell.mapping);

	queuer_rsc->QueueId = (uint64_t)kmt_queue;
	queuer_rsc->Queue_DoorBell = (uint32_t *)((char*)doorbell_addr + doorbell_offset);

	printf("\tQueueResource->QueueId        = 0x%016lX.\n", queuer_rsc->QueueId);
	printf("\tQueueResource->Queue_DoorBell = 0x%016lX.\n", queuer_rsc->Queue_DoorBell);

	//printf("\n+++++++++++++++++++++++\n");
	//printf("doorbell size        = 0x%016lX.\n", doorbell.size);
	//printf("args.doorbell offset = 0x%016lX.\n", args.doorbell_offset);
	//printf("doorbell mmap offset = 0x%016lX.\n", doorbell_mmap_offset);
	//printf("doorbell offset      = 0x%016lX.\n", doorbell_offset);
	//printf("doorbell alloc addr  = 0x%016lX.\n", doorbell_addr);
	//printf("queue doorbell addr  = 0x%016lX.\n", queuer_rsc->Queue_DoorBell);
}
void KmtDestroyQueue(uint64_t QueueId)
{
	kmt_queue_t * q = (kmt_queue_t*)(QueueId);
	struct kfd_ioctl_destroy_queue_args args = { 0 };
	
	args.queue_id = q->queue_id;

	int err = kmtIoctl(gKfdFd, AMDKFD_IOC_DESTROY_QUEUE, &args);
	assert(err != -1);

	//free_exec_aligned_memory(q->eop_buffer, q->dev_info->eop_buffer_size, PAGE_SIZE, q->use_ats);
	//free_exec_aligned_memory(q->ctx_save_restore, q->ctx_save_restore_size, PAGE_SIZE, q->use_ats);
	//free_exec_aligned_memory((void *)q, sizeof(*q), PAGE_SIZE, q->use_ats);
}

#if 0

#define HSA_QUEUE_MAX_PKT_NUM (128*1024)
typedef struct hsa_signal_s
{
	uint64_t handle;
} hsa_signal_t;
typedef struct hsa_queue_s
{
	uint32_t type;
	uint32_t features;

	void * base_address;
	uint32_t reserved0;
	hsa_signal_t doorbell_signal;
	uint32_t size;
	uint32_t reserved1;
	uint64_t id;

} hsa_queue_t;

typedef struct __attribute__((aligned(64))) amd_queue_s
{
	hsa_queue_t hsa_queue;
	uint32_t reserved1[4];
	volatile uint64_t write_dispatch_id;
	uint32_t group_segment_aperture_base_hi;
	uint32_t private_segment_aperture_base_hi;
	uint32_t max_cu_id;
	uint32_t max_wave_id;
	volatile uint64_t max_legacy_doorbell_dispatch_id_plus_1;
	volatile uint32_t legacy_doorbell_lock;
	uint32_t reserved2[9];
	volatile uint64_t read_dispatch_id;
	uint32_t read_dispatch_id_field_base_byte_offset;
	uint32_t compute_tmpring_size;
	uint32_t scratch_resource_descriptor[4];
	uint64_t scratch_backing_memory_location;
	uint64_t scratch_backing_memory_byte_size;
	uint32_t scratch_workitem_byte_size;
	uint32_t queue_properties; // amd_queue_properties_t
	uint32_t reserved3[2];
	hsa_signal_t queue_inactive_signal;
	uint32_t reserved4[14];
} amd_queue_t;
typedef struct __attribute__((aligned(64))) amd_signal_s
{
	int64_t kind;//amd_signal_kind64_t
	union
	{
		volatile int64_t value;
		volatile uint32_t* legacy_hardware_doorbell_ptr;
		volatile uint64_t* hardware_doorbell_ptr;
	};
	uint64_t event_mailbox_ptr;
	uint32_t event_id;
	uint32_t reserved1;
	uint64_t start_ts;
	uint64_t end_ts;
	union {
		amd_queue_t* queue_ptr;
		uint64_t reserved2;
	};
	uint32_t reserved3[2];
} amd_signal_t;

typedef struct hsa_kernel_dispatch_packet_s
{
	uint16_t header;
	uint16_t setup;
	uint16_t workgroup_size_x;
	uint16_t workgroup_size_y;
	uint16_t workgroup_size_z;
	uint16_t reserved0;
	uint32_t grid_size_x;
	uint32_t grid_size_y;
	uint32_t grid_size_z;
	uint32_t private_segment_size;
	uint32_t group_segment_size;
	uint64_t kernel_object;
	void* kernarg_address;
	uint32_t reserved1;
	uint64_t reserved2;
	hsa_signal_t completion_signal;

} hsa_kernel_dispatch_packet_t;
typedef struct hsa_agent_dispatch_packet_s
{
	uint16_t header;
	uint16_t type;
	uint32_t reserved0;
	void* return_address;
	uint32_t reserved1;
	uint64_t arg[4];
	uint64_t reserved2;
	hsa_signal_t completion_signal;

} hsa_agent_dispatch_packet_t;
typedef struct hsa_barrier_and_packet_s
{
	uint16_t header;
	uint16_t reserved0;
	uint32_t reserved1;
	hsa_signal_t dep_signal[5];
	uint64_t reserved2;
	hsa_signal_t completion_signal;

} hsa_barrier_and_packet_t;
typedef struct hsa_barrier_or_packet_s
{
	uint16_t header;
	uint16_t reserved0;
	uint32_t reserved1;
	hsa_signal_t dep_signal[5];
	uint64_t reserved2;
	hsa_signal_t completion_signal;

} hsa_barrier_or_packet_t;
struct AqlPacket // 72Byte size
{
	union
	{
		hsa_kernel_dispatch_packet_t dispatch;
		hsa_barrier_and_packet_t barrier_and;
		hsa_barrier_or_packet_t barrier_or;
		hsa_agent_dispatch_packet_t agent;
	};

	uint8_t type() const
	{
		//return ((dispatch.header >> HSA_PACKET_HEADER_TYPE) & ((1 << HSA_PACKET_HEADER_WIDTH_TYPE) - 1));
		return ((dispatch.header >> 0) & ((1 << 8) - 1));
	}
	bool IsValid() const
	{
		//return (type() <= HSA_PACKET_TYPE_BARRIER_OR) & (type() != HSA_PACKET_TYPE_INVALID);
		return (type() <= 5) & (type() != 1);
	}

	std::string string() const
	{
		std::stringstream string;
		uint8_t type = this->type();

		const char* type_names[] =
		{
			"HSA_PACKET_TYPE_VENDOR_SPECIFIC", "HSA_PACKET_TYPE_INVALID",
			"HSA_PACKET_TYPE_KERNEL_DISPATCH", "HSA_PACKET_TYPE_BARRIER_AND",
			"HSA_PACKET_TYPE_AGENT_DISPATCH",  "HSA_PACKET_TYPE_BARRIER_OR"
		};

		/*string << "type: " << type_names[type]
			<< "\nbarrier: " << ((dispatch.header >> HSA_PACKET_HEADER_BARRIER) & ((1 << HSA_PACKET_HEADER_WIDTH_BARRIER) - 1))
			<< "\nacquire: " << ((dispatch.header >> HSA_PACKET_HEADER_SCACQUIRE_FENCE_SCOPE) & ((1 << HSA_PACKET_HEADER_WIDTH_SCACQUIRE_FENCE_SCOPE) - 1))
			<< "\nrelease: " << ((dispatch.header >> HSA_PACKET_HEADER_SCRELEASE_FENCE_SCOPE) & ((1 << HSA_PACKET_HEADER_WIDTH_SCRELEASE_FENCE_SCOPE) - 1));

		if (type == HSA_PACKET_TYPE_KERNEL_DISPATCH)
		{
			string << "\nDim: " << dispatch.setup
				<< "\nworkgroup_size: " << dispatch.workgroup_size_x << ", " << dispatch.workgroup_size_y << ", " << dispatch.workgroup_size_z
				<< "\ngrid_size: " << dispatch.grid_size_x << ", " << dispatch.grid_size_y << ", " << dispatch.grid_size_z
				<< "\nprivate_size: " << dispatch.private_segment_size
				<< "\ngroup_size: " << dispatch.group_segment_size
				<< "\nkernel_object: " << dispatch.kernel_object
				<< "\nkern_arg: " << dispatch.kernarg_address
				<< "\nsignal: " << dispatch.completion_signal.handle;
		}

		if ((type == HSA_PACKET_TYPE_BARRIER_AND) || (type == HSA_PACKET_TYPE_BARRIER_OR))
		{
			for (int i = 0; i < 5; i++)
				string << "\ndep[" << i << "]: " << barrier_and.dep_signal[i].handle;
			string << "\nsignal: " << barrier_and.completion_signal.handle;
		}*/

		return string.str();
	}
};

void * ring_buffer;// ring_buf
uint32_t queue_pkt_num; // 128K个packets
uint32_t queue_size; // ring_buf_alloc_bytes_
void * pm4_ib_buffer;// pm4_ib_buf_
uint32_t pm4_ib_size;// pm4_ib_size_b_
void allocate_ring_buffer()
{
	uint32_t queue_min_pkt_num = 0x400 / sizeof(AqlPacket);
	uint32_t queue_max_pkt_num = 0x100000000 / sizeof(AqlPacket);
	queue_pkt_num = 32;// HSA_QUEUE_MAX_PKT_NUM; // 128K个packets
	queue_size = queue_pkt_num * sizeof(AqlPacket); // ring_buf_alloc_bytes_

	ring_buffer = HsaAllocCPU(queue_size);

	for (uint32_t i = 0; i < queue_pkt_num; ++i)
	{
		((uint32_t*)ring_buffer)[16 * i] = 1;// HSA_PACKET_TYPE_INVALID;
	}

	printf("\n\t-----------------------\n");
	printf("\tallocate ring buffer:\n");
	printf("\t\taql packet size = %d(Byte).\n", sizeof(AqlPacket));
	printf("\t\thsa queue min packet number = %d.\n", queue_min_pkt_num);
	printf("\t\thsa queue max packet number = %.3f(M).\n", queue_max_pkt_num / 1024.0 / 1024.0);
	printf("\t\thsa queue user set packet number = %.3f(K).\n", queue_pkt_num / 1024.0);
	printf("\t\thsa queue size = %.3f(KB).\n", queue_size / 1024.0);
	printf("\t\tring buffer address = 0x%016lX.\n", ring_buffer);
}

amd_signal_t amd_signal;
amd_queue_t amd_queue;
HsaQueueResource queue_resource;
void hsa_create_queue()
{
	printf("\n=======================\n");
	printf("Create Queue.\n");

	allocate_ring_buffer();

	queue_resource.Queue_read_ptr_aql = (uint64_t*)&amd_queue.read_dispatch_id;
	queue_resource.Queue_write_ptr_aql = (uint64_t*)&amd_queue.write_dispatch_id;

	KmtCreateQueue(KFD_IOC_QUEUE_TYPE_COMPUTE_AQL, ring_buffer, queue_size, &queue_resource);
	return;

	amd_signal.kind = -1;// AMD_SIGNAL_KIND_DOORBELL
	amd_signal.legacy_hardware_doorbell_ptr = (volatile uint32_t*)queue_resource.Queue_DoorBell;
	amd_signal.queue_ptr = &amd_queue;

	// Populate amd_queue_ structure.
	amd_queue.hsa_queue.type = 0;// HSA_QUEUE_TYPE_MULTI;
	amd_queue.hsa_queue.features = 1;// HSA_QUEUE_FEATURE_KERNEL_DISPATCH;
	amd_queue.hsa_queue.base_address = ring_buffer;
	//amd_queue.hsa_queue.doorbell_signal = Signal::Convert(this);
	amd_queue.hsa_queue.size = queue_pkt_num;
	amd_queue.hsa_queue.id = queue_resource.QueueId;
	amd_queue.read_dispatch_id_field_base_byte_offset = uint32_t(uintptr_t(&amd_queue.read_dispatch_id) - uintptr_t(&amd_queue));

	amd_queue.max_cu_id = vega20_cu_num - 1;
	amd_queue.max_wave_id = (max_wave_per_simd * 4) - 1;

	// create event

	// allocate pm4 buffer
	pm4_ib_size = 0x1000;

	HsaMemFlags memFlag;
	memFlag.ui32.PageSize = HSA_PAGE_SIZE_4KB;
	memFlag.ui32.NoSubstitute = 1;
	memFlag.ui32.HostAccess = 1;
	memFlag.ui32.NonPaged = 1;
	memFlag.ui32.CoarseGrain = 1;

	// allocate device
	//pm4_ib_buffer = fmm_allocate_device(pm4_ib_size, memFlag);
}
#endif


// doorbell_type_ = 2
// profile_ = HSA_PROFILE_BASE
// queue_full_workaround_ = false
// use_ats = props.NumCPUCores = false
// zfb_support = env(HSA_ZBF) = false
// Type == HSA_QUEUE_COMPUTE_AQL

/*
user:
	hsa_queue_create():
		size = queue_size = HSA_AGENT_INFO_QUEUE_MAX_SIZE = 0x20000;
		type = HSA_QUEUE_TYPE_MULTI;
		*callback = *data = null;
		private_segment_size = UINT32_MAX = 0;
		group_segment_size = UINT32_MAX; not used
		
hsa:
	new AqlQueue():
		size = 128KB
		max_pkts = 0x100000000 / sizeof(AqlPacket)

	//AllocRegisteredRingBuffer();
	//	core::Runtime::runtime_singleton_->system_allocator()
	//	core::Runtime::runtime_singleton_->system_allocator_
	//	core::Runtime::runtime_singleton_->AllocateMemory()
	//	region->Allocate();
	//		AllocateKfdMemory() = hsaKmtAllocMemory()
	//		MakeKfdMemoryResident() = hsaKmtMapMemoryToGPUNodes()
	//	hsaKmtCreateQueue()
*/

/*
初始化 MemoryRegion:	
	if (IsLocalMemory())
	{
		mem_flag.PageSize = HSA_PAGE_SIZE_4KB;
		mem_flag.NoSubstitute = 1;
		mem_flag.HostAccess = (mem_props_.HeapType == HSA_HEAPTYPE_FRAME_BUFFER_PRIVATE) ? 0 : 1;
		mem_flag.NonPaged = 1;
	}
	if (IsSystem()) 
	{
		mem_flag.PageSize = HSA_PAGE_SIZE_4KB;
		mem_flag.NoSubstitute = 1;
		mem_flag.HostAccess = 1;
		mem_flag.CachePolicy = HSA_CACHING_CACHED;
	}

	fine_grain = 0;
	mem_flag_.ui32.CoarseGrain = (fine_grain) ? 0 : 1;
*/

