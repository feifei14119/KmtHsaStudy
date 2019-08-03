#include <string>
#include <iostream>
#include <vector>

#include "hsa_test.h"

using namespace std;

static const uint32_t num_cu = 64;

/* hsa定义的常量 */
static const uint32_t minAqlSize = 0x1000;   // 4KB min
static const uint32_t maxAqlSize = 0x20000;  // 8MB max


typedef struct hsa_signal_s 
{
	uint64_t handle;
} hsa_signal_t; 
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

	uint64_t reserved2;

	hsa_signal_t completion_signal;
} hsa_kernel_dispatch_packet_t;
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
typedef struct hsa_agent_dispatch_packet_s 
{
	uint16_t header;
	uint16_t type;

	uint32_t reserved0;

	void* return_address;

	uint64_t arg[4];

	uint64_t reserved2;

	hsa_signal_t completion_signal;
} hsa_agent_dispatch_packet_t;
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

// AMD Queue.
//#define AMD_QUEUE_ALIGN_BYTES 64
//#define AMD_QUEUE_ALIGN __ALIGNED__(AMD_QUEUE_ALIGN_BYTES)
typedef struct amd_queue_s 
{
	//hsa_queue_t hsa_queue;
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
	//amd_queue_properties32_t queue_properties;
	uint32_t reserved3[2];
	hsa_signal_t queue_inactive_signal;
	uint32_t reserved4[14];
} amd_queue_t;

/* 创建时用户传入的参数 */
static uint32_t queue_size = minAqlSize;
static uint32_t private_segment_size = 8; // size per thread
static uint32_t group_segment_size = 0;

void create_queue_test()
{
	size_t scratch_size = private_segment_size * 32 * 64 * num_cu;

	if (scratch_size != 0) 
	{
		//AcquireQueueScratch(scratch);
	}

	uint32_t min_bytes = 0x400; // 1KB
	uint64_t max_bytes = 0x100000000; // 4GB
	const uint32_t min_pkts = min_bytes / sizeof(AqlPacket);
	const uint32_t max_pkts = max_bytes / sizeof(AqlPacket);
	printf("packet size: %d\n", sizeof(AqlPacket));
	printf("min pkt num: %d\n", min_pkts);
	printf("max pkt num: %d\n", max_pkts);

	uint32_t queue_size_pkts = queue_size;
	uint32_t queue_size_bytes = queue_size_pkts * sizeof(AqlPacket);
	printf("queue size: 0x%08X\n", queue_size_bytes);
	
	void* ring_buff;
	uint32_t ring_buf_phys_size_bytes = queue_size_bytes;
	uint32_t ring_buf_alloc_bytes = 2 * ring_buf_phys_size_bytes;
	ring_buff = malloc(queue_size_bytes);

	HsaQueueResource queue_rsrc;
	amd_queue_t amd_queue;
	queue_rsrc.Queue_read_ptr_aql = (uint64_t*)&amd_queue.read_dispatch_id;
	queue_rsrc.Queue_write_ptr_aql = (uint64_t*)&amd_queue.write_dispatch_id;

	HSAKMT_STATUS t;
	t = hsaKmtCreateQueue(1, HSA_QUEUE_COMPUTE_AQL, 100, HSA_QUEUE_PRIORITY_NORMAL, ring_buff, ring_buf_alloc_bytes, NULL, &queue_rsrc);
	printf("queue id: 0x%08X\n", queue_rsrc.QueueId);
	printf("read dispatch id: %d\n", amd_queue.read_dispatch_id);
	printf("write dispatch id: %d\n", amd_queue.write_dispatch_id);
}

void hsa_queue_test()
{
	printf("\n***********************\n");
	hsa_test_open();
	create_queue_test();
}
