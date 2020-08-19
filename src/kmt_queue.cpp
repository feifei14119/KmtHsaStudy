#include "kmt.h"

uint32_t vega20_eop_buff_size = 4096;
uint32_t vega20_doorbell_size = 8;
uint32_t vega20_cu_num = 60;
uint32_t wave_per_cu = 32;
uint32_t max_wave_per_simd = 10; // for vega20
uint32_t wg_ctx_data_size_per_cu = 344576;
//static uint32_t priority_map[] = { 0, 3, 5, 7, 9, 11, 15 }; priority = priority_map[prio_idx + 3]; default_idx = 0;

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
	printf("\tkmtIoctl create queue.\n");
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
	printf("\t\t-----------------------\n");
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
	printf("\t\t-----------------------\n");
	printf("\t\tdoorbell.mapping = 0x%016lX.\n", doorbell.mapping);

	queuer_rsc->QueueId = (uint64_t)kmt_queue;
	queuer_rsc->Queue_DoorBell = (uint32_t *)((char*)doorbell_addr + doorbell_offset);
	
	printf("\n");
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

