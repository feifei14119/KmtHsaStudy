#include "kmthsa.h"

#define AQL_PACKET_MAX_NUM	(1024)
#define KERNEL_ARG_MAX_SIZE	(1024)

typedef struct hsa_signal_s 
{
	/**
	 * Opaque handle. Two handles reference the same object of the enclosing type
	 * if and only if they are equal. The value 0 is reserved.
	 */
	uint64_t handle;
} hsa_signal_t;
typedef struct hsa_kernel_dispatch_packet_s 
{
	/**
	 * Packet header. Used to configure multiple packet parameters such as the
	 * packet type. The parameters are described by ::hsa_packet_header_t.
	 */
	uint16_t header;

	/**
	 * Dispatch setup parameters. Used to configure kernel dispatch parameters
	 * such as the number of dimensions in the grid. The parameters are described
	 * by ::hsa_kernel_dispatch_packet_setup_t.
	 */
	uint16_t setup;

	/**
	 * X dimension of work-group, in work-items. Must be greater than 0.
	 */
	uint16_t workgroup_size_x;

	/**
	 * Y dimension of work-group, in work-items. Must be greater than
	 * 0. If the grid has 1 dimension, the only valid value is 1.
	 */
	uint16_t workgroup_size_y;

	/**
	 * Z dimension of work-group, in work-items. Must be greater than
	 * 0. If the grid has 1 or 2 dimensions, the only valid value is 1.
	 */
	uint16_t workgroup_size_z;

	/**
	 * Reserved. Must be 0.
	 */
	uint16_t reserved0;

	/**
	 * X dimension of grid, in work-items. Must be greater than 0. Must
	 * not be smaller than @a workgroup_size_x.
	 */
	uint32_t grid_size_x;

	/**
	 * Y dimension of grid, in work-items. Must be greater than 0. If the grid has
	 * 1 dimension, the only valid value is 1. Must not be smaller than @a
	 * workgroup_size_y.
	 */
	uint32_t grid_size_y;

	/**
	 * Z dimension of grid, in work-items. Must be greater than 0. If the grid has
	 * 1 or 2 dimensions, the only valid value is 1. Must not be smaller than @a
	 * workgroup_size_z.
	 */
	uint32_t grid_size_z;

	/**
	 * Size in bytes of private memory allocation request (per work-item).
	 */
	uint32_t private_segment_size;

	/**
	 * Size in bytes of group memory allocation request (per work-group). Must not
	 * be less than the sum of the group memory used by the kernel (and the
	 * functions it calls directly or indirectly) and the dynamically allocated
	 * group segment variables.
	 */
	uint32_t group_segment_size;

	/**
	 * Opaque handle to a code object that includes an implementation-defined
	 * executable code for the kernel.
	 */
	uint64_t kernel_object;

	void* kernarg_address;

	/**
	 * Reserved. Must be 0.
	 */
	uint64_t reserved2;

	/**
	 * Signal used to indicate completion of the job. The application can use the
	 * special signal handle 0 to indicate that no signal is used.
	 */
	hsa_signal_t completion_signal;

} hsa_kernel_dispatch_packet_t;
typedef struct hsa_agent_dispatch_packet_s {
	/**
	 * Packet header. Used to configure multiple packet parameters such as the
	 * packet type. The parameters are described by ::hsa_packet_header_t.
	 */
	uint16_t header;

	/**
	 * Application-defined function to be performed by the destination agent.
	 */
	uint16_t type;

	/**
	 * Reserved. Must be 0.
	 */
	uint32_t reserved0;

	void* return_address;

	/**
	 * Function arguments.
	 */
	uint64_t arg[4];

	/**
	 * Reserved. Must be 0.
	 */
	uint64_t reserved2;

	/**
	 * Signal used to indicate completion of the job. The application can use the
	 * special signal handle 0 to indicate that no signal is used.
	 */
	hsa_signal_t completion_signal;

} hsa_agent_dispatch_packet_t;
typedef struct hsa_barrier_and_packet_s {
	/**
	 * Packet header. Used to configure multiple packet parameters such as the
	 * packet type. The parameters are described by ::hsa_packet_header_t.
	 */
	uint16_t header;

	/**
	 * Reserved. Must be 0.
	 */
	uint16_t reserved0;

	/**
	 * Reserved. Must be 0.
	 */
	uint32_t reserved1;

	/**
	 * Array of dependent signal objects. Signals with a handle value of 0 are
	 * allowed and are interpreted by the packet processor as satisfied
	 * dependencies.
	 */
	hsa_signal_t dep_signal[5];

	/**
	 * Reserved. Must be 0.
	 */
	uint64_t reserved2;

	/**
	 * Signal used to indicate completion of the job. The application can use the
	 * special signal handle 0 to indicate that no signal is used.
	 */
	hsa_signal_t completion_signal;

} hsa_barrier_and_packet_t;
typedef struct hsa_barrier_or_packet_s {
	/**
	 * Packet header. Used to configure multiple packet parameters such as the
	 * packet type. The parameters are described by ::hsa_packet_header_t.
	 */
	uint16_t header;

	/**
	 * Reserved. Must be 0.
	 */
	uint16_t reserved0;

	/**
	 * Reserved. Must be 0.
	 */
	uint32_t reserved1;

	/**
	 * Array of dependent signal objects. Signals with a handle value of 0 are
	 * allowed and are interpreted by the packet processor as dependencies not
	 * satisfied.
	 */
	hsa_signal_t dep_signal[5];

	/**
	 * Reserved. Must be 0.
	 */
	uint64_t reserved2;

	/**
	 * Signal used to indicate completion of the job. The application can use the
	 * special signal handle 0 to indicate that no signal is used.
	 */
	hsa_signal_t completion_signal;

} hsa_barrier_or_packet_t;

struct AqlPacket 
{
	union 
	{
		hsa_kernel_dispatch_packet_t dispatch;
		hsa_barrier_and_packet_t barrier_and;
		hsa_barrier_or_packet_t barrier_or;
		hsa_agent_dispatch_packet_t agent;
	};
};

// ==================================================================

static uint32_t CommandSize = 0;
static void * AqlQueueRingBuff; // ring buffer address for aql queue
static HsaQueueResource AqlQueueResource;
static hsa_kernel_dispatch_packet_t * AqlPkt;

uint32_t KernelArgsSize;
void * KernelArgsAddress;
void * KernelArgsPtr;

volatile uint64_t read_dispatch_id;
volatile uint64_t write_dispatch_id;

// ==================================================================

void HsaAqlCreate()
{
	printf("HsaAqlCreate\n");

	CommandSize = 0;

	uint64_t rb_size = AQL_PACKET_MAX_NUM * sizeof(AqlPacket);
	AqlQueueRingBuff = HsaAllocCPU(rb_size);
	for (uint32_t i = 0; i < AQL_PACKET_MAX_NUM; i++)
	{
		((hsa_kernel_dispatch_packet_t*)AqlQueueRingBuff)[i].header = 1;// HSA_PACKET_TYPE_INVALID
	}

	AqlQueueResource.Queue_read_ptr_aql = (uint64_t*)(&read_dispatch_id);
	AqlQueueResource.Queue_write_ptr_aql = (uint64_t*)(&write_dispatch_id);
	KmtCreateQueue(KFD_IOC_QUEUE_TYPE_COMPUTE_AQL, AqlQueueRingBuff, rb_size, &AqlQueueResource);

	printf("\tqueue id      = 0x%016lX.\n", AqlQueueResource.QueueId);
	//printf("\twrite pointer = 0x%016lX, Value = %d.\n", AqlQueueResource.QueueWptrValue, *AqlQueueResource.Queue_write_ptr);
	//printf("\tread  pointer = 0x%016lX, Value = %d.\n", AqlQueueResource.QueueRptrValue, *AqlQueueResource.Queue_read_ptr);
	printf("\twrite pointer = 0x%016lX.\n", AqlQueueResource.QueueWptrValue);
	printf("\tread  pointer = 0x%016lX.\n", AqlQueueResource.QueueRptrValue);
	printf("\tringbuff addr = 0x%016lX.\n", AqlQueueRingBuff);
	printf("\tdoorbell addr = 0x%016lX.\n", AqlQueueResource.QueueDoorBell);

	AqlPkt = (hsa_kernel_dispatch_packet_t*)AqlQueueRingBuff;
	memset(AqlPkt, 0, sizeof(AqlPacket));
	printf("alloc rb addr = 0x%016lX, size = %ld\n", AqlQueueRingBuff, rb_size);

	KernelArgsSize = 1024;
	KernelArgsAddress = HsaAllocCPU(KERNEL_ARG_MAX_SIZE);
	KernelArgsPtr = KernelArgsAddress;

	//printf("\tqueue id      = 0x%016lX.\n", AqlQueueResource.QueueId);
	//printf("\twrite pointer = 0x%016lX, Value = %d.\n", AqlQueueResource.QueueWptrValue, *AqlQueueResource.Queue_write_ptr);
	//printf("\tread  pointer = 0x%016lX, Value = %d.\n", AqlQueueResource.QueueRptrValue, *AqlQueueResource.Queue_read_ptr);
	//printf("\tringbuff addr = 0x%016lX.\n", DmaQueueRingBuff);
	//printf("\tdoorbell addr = 0x%016lX.\n", AqlQueueResource.QueueDoorBell);
	printf("\n");
}

void HsaAqlSetPkt(void * kernelHandle, uint32_t grpSz, uint32_t grdSz)
{
	printf("HsaAqlSetPkt\n");
	uint16_t header =
		(2 << 0) | //(HSA_PACKET_TYPE_KERNEL_DISPATCH << HSA_PACKET_HEADER_TYPE) |
		(1 << 8) | //(1 << HSA_PACKET_HEADER_BARRIER) |
		(2 << 9) | //(HSA_FENCE_SCOPE_SYSTEM << HSA_PACKET_HEADER_ACQUIRE_FENCE_SCOPE) |
		(2 << 11); //(HSA_FENCE_SCOPE_SYSTEM << HSA_PACKET_HEADER_RELEASE_FENCE_SCOPE);
	AqlPkt->header = header;

	AqlPkt->workgroup_size_x = grpSz;
	AqlPkt->workgroup_size_y = 1;
	AqlPkt->workgroup_size_z = 1;
	AqlPkt->grid_size_x = grdSz;
	AqlPkt->grid_size_y = 1;
	AqlPkt->grid_size_z = 1;
	uint16_t dim = 1;
	AqlPkt->setup = dim << 0; // dim << HSA_KERNEL_DISPATCH_PACKET_SETUP_DIMENSIONS;

	AqlPkt->group_segment_size = 0;
	AqlPkt->private_segment_size = 0;

	AqlPkt->kernel_object = (uint64_t)kernelHandle;
	AqlPkt->kernarg_address = KernelArgsAddress;
	//AqlPkt->completion_signal = signal;
}

void HsaAqlSetKernelArg(void ** arg, size_t argSize)
{
	printf("HsaAqlSetKernelArg\n");
	memcpy(KernelArgsPtr, arg, argSize);
	printf("dst = 0x%016lX, src = 0x%016lX, size = %ld\n", KernelArgsPtr, arg, argSize);
	KernelArgsPtr = (char *)KernelArgsPtr + argSize;
}

void HsaAqlRingDoorbell()
{
	printf("HsaAqlRingDoorbell\n");
	*(AqlQueueResource.Queue_DoorBell_aql) = 0;
}

// ==================================================================

void RunAqlTest()
{
	HsaAqlCreate();
}
