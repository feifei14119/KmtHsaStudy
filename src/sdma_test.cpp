#include "kmt_test.h"

static HsaQueueResource QueueResource;
#define MAX_SDMA_COPY_SIZE (0x3fffe0) // 4M - 32
#define SDMA_RINGBUFF_SIZE (1024 * 1024)
static uint32_t CommandSize = 0;
static void * DmaQueueRingBuff; // ring buffer address for sdma copy queue
static uint64_t ReserveIdx, CommitIdx, DoorBellIdx;
static HsaEvent * SdmaEvent;
static uint64_t InitialValue = 0;

typedef struct SDMA_PKT_COPY_LINEAR_TAG {
	union {
		struct {
			unsigned int op : 8;
			unsigned int sub_op : 8;
			unsigned int extra_info : 16;
		};
		unsigned int DW_0_DATA;
	} HEADER_UNION;

	union {
		struct {
			unsigned int count : 22;
			unsigned int reserved_0 : 10;
		};
		unsigned int DW_1_DATA;
	} COUNT_UNION;

	union {
		struct {
			unsigned int reserved_0 : 16;
			unsigned int dst_swap : 2;
			unsigned int reserved_1 : 6;
			unsigned int src_swap : 2;
			unsigned int reserved_2 : 6;
		};
		unsigned int DW_2_DATA;
	} PARAMETER_UNION;

	union {
		struct {
			unsigned int src_addr_31_0 : 32;
		};
		unsigned int DW_3_DATA;
	} SRC_ADDR_LO_UNION;

	union {
		struct {
			unsigned int src_addr_63_32 : 32;
		};
		unsigned int DW_4_DATA;
	} SRC_ADDR_HI_UNION;

	union {
		struct {
			unsigned int dst_addr_31_0 : 32;
		};
		unsigned int DW_5_DATA;
	} DST_ADDR_LO_UNION;

	union {
		struct {
			unsigned int dst_addr_63_32 : 32;
		};
		unsigned int DW_6_DATA;
	} DST_ADDR_HI_UNION;

	static const size_t kMaxSize_ = 0x3fffe0;
} SDMA_PKT_COPY_LINEAR; 
typedef struct SDMA_PKT_POLL_REGMEM_TAG {
	union {
		struct {
			unsigned int op : 8;
			unsigned int sub_op : 8;
			unsigned int reserved_0 : 10;
			unsigned int hdp_flush : 1;
			unsigned int reserved_1 : 1;
			unsigned int func : 3;
			unsigned int mem_poll : 1;
		};
		unsigned int DW_0_DATA;
	} HEADER_UNION;

	union {
		struct {
			unsigned int addr_31_0 : 32;
		};
		unsigned int DW_1_DATA;
	} ADDR_LO_UNION;

	union {
		struct {
			unsigned int addr_63_32 : 32;
		};
		unsigned int DW_2_DATA;
	} ADDR_HI_UNION;

	union {
		struct {
			unsigned int value : 32;
		};
		unsigned int DW_3_DATA;
	} VALUE_UNION;

	union {
		struct {
			unsigned int mask : 32;
		};
		unsigned int DW_4_DATA;
	} MASK_UNION;

	union {
		struct {
			unsigned int interval : 16;
			unsigned int retry_count : 12;
			unsigned int reserved_0 : 4;
		};
		unsigned int DW_5_DATA;
	} DW5_UNION;
} SDMA_PKT_POLL_REGMEM;
typedef struct SDMA_PKT_HDP_FLUSH_TAG {
	unsigned int DW_0_DATA;
	unsigned int DW_1_DATA;
	unsigned int DW_2_DATA;
	unsigned int DW_3_DATA;
	unsigned int DW_4_DATA;
	unsigned int DW_5_DATA;

	// Version of gfx9 sDMA microcode introducing SDMA_PKT_HDP_FLUSH
	static const uint16_t kMinVersion_ = 0x1A5;
} SDMA_PKT_HDP_FLUSH;
typedef struct SDMA_PKT_ATOMIC_TAG {
	union {
		struct {
			unsigned int op : 8;
			unsigned int sub_op : 8;
			unsigned int l : 1;
			unsigned int reserved_0 : 8;
			unsigned int operation : 7;
		};
		unsigned int DW_0_DATA;
	} HEADER_UNION;

	union {
		struct {
			unsigned int addr_31_0 : 32;
		};
		unsigned int DW_1_DATA;
	} ADDR_LO_UNION;

	union {
		struct {
			unsigned int addr_63_32 : 32;
		};
		unsigned int DW_2_DATA;
	} ADDR_HI_UNION;

	union {
		struct {
			unsigned int src_data_31_0 : 32;
		};
		unsigned int DW_3_DATA;
	} SRC_DATA_LO_UNION;

	union {
		struct {
			unsigned int src_data_63_32 : 32;
		};
		unsigned int DW_4_DATA;
	} SRC_DATA_HI_UNION;

	union {
		struct {
			unsigned int cmp_data_31_0 : 32;
		};
		unsigned int DW_5_DATA;
	} CMP_DATA_LO_UNION;

	union {
		struct {
			unsigned int cmp_data_63_32 : 32;
		};
		unsigned int DW_6_DATA;
	} CMP_DATA_HI_UNION;

	union {
		struct {
			unsigned int loop_interval : 13;
			unsigned int reserved_0 : 19;
		};
		unsigned int DW_7_DATA;
	} LOOP_UNION;
} SDMA_PKT_ATOMIC;
typedef struct SDMA_PKT_FENCE_TAG {
	union {
		struct {
			unsigned int op : 8;
			unsigned int sub_op : 8;
			unsigned int reserved_0 : 16;
		};
		unsigned int DW_0_DATA;
	} HEADER_UNION;

	union {
		struct {
			unsigned int addr_31_0 : 32;
		};
		unsigned int DW_1_DATA;
	} ADDR_LO_UNION;

	union {
		struct {
			unsigned int addr_63_32 : 32;
		};
		unsigned int DW_2_DATA;
	} ADDR_HI_UNION;

	union {
		struct {
			unsigned int data : 32;
		};
		unsigned int DW_3_DATA;
	} DATA_UNION;
} SDMA_PKT_FENCE;
typedef struct SDMA_PKT_TRAP_TAG {
	union {
		struct {
			unsigned int op : 8;
			unsigned int sub_op : 8;
			unsigned int reserved_0 : 16;
		};
		unsigned int DW_0_DATA;
	} HEADER_UNION;

	union {
		struct {
			unsigned int int_ctx : 28;
			unsigned int reserved_1 : 4;
		};
		unsigned int DW_1_DATA;
	} INT_CONTEXT_UNION;
} SDMA_PKT_TRAP;

static void build_copy_cmd(void * src, void * dst, uint32_t copy_size)
{
	assert(copy_size < MAX_SDMA_COPY_SIZE);

	SDMA_PKT_COPY_LINEAR dma_copy_cmd;
	memset(&dma_copy_cmd, 0, sizeof(SDMA_PKT_COPY_LINEAR));

	dma_copy_cmd.HEADER_UNION.op = 1;// SDMA_OP_COPY;
	dma_copy_cmd.HEADER_UNION.sub_op = 0;// SDMA_SUBOP_COPY_LINEAR;
	dma_copy_cmd.COUNT_UNION.count = copy_size;
	dma_copy_cmd.SRC_ADDR_LO_UNION.src_addr_31_0 = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(src));
	dma_copy_cmd.SRC_ADDR_HI_UNION.src_addr_63_32 = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(src) >> 32);
	dma_copy_cmd.DST_ADDR_LO_UNION.dst_addr_31_0 = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(dst));
	dma_copy_cmd.DST_ADDR_HI_UNION.dst_addr_63_32 = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(dst) >> 32);

	char * pbuff = (char*)DmaQueueRingBuff + CommandSize;
	memcpy(pbuff, &dma_copy_cmd, sizeof(SDMA_PKT_COPY_LINEAR));
	CommandSize += sizeof(SDMA_PKT_COPY_LINEAR);

	printf("\n\t-----------------------\n");
	printf("\tbuild copy command.\n");
	printf("\tsize = %d.\n", sizeof(SDMA_PKT_COPY_LINEAR));
	for (uint32_t i = 0; i < sizeof(SDMA_PKT_COPY_LINEAR); i++)
	{
		printf("0x%02X, ", *(((unsigned char*)(&dma_copy_cmd)) + i));
		if ((i + 1) % 8 == 0)
			printf("\n");
	}
	printf("\n");
}
static void build_poll_cmd()
{
	SDMA_PKT_POLL_REGMEM poll_cmd;
	memset(&poll_cmd, 0, sizeof(SDMA_PKT_POLL_REGMEM));

	poll_cmd.HEADER_UNION.op = 8;// SDMA_OP_POLL_REGMEM;
	poll_cmd.HEADER_UNION.mem_poll = 1;
	poll_cmd.HEADER_UNION.func = 0x3;  // IsEqual.
	poll_cmd.ADDR_LO_UNION.addr_31_0 = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(&InitialValue));
	poll_cmd.ADDR_HI_UNION.addr_63_32 = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(&InitialValue) >> 32);
	poll_cmd.VALUE_UNION.value = InitialValue;
	poll_cmd.MASK_UNION.mask = 0xffffffff;  // Compare the whole content.
	poll_cmd.DW5_UNION.interval = 0x04;
	poll_cmd.DW5_UNION.retry_count = 0xfff;  // Retry forever.

	char * pbuff = (char*)DmaQueueRingBuff + CommandSize;
	memcpy(pbuff, &poll_cmd, sizeof(SDMA_PKT_POLL_REGMEM));
	CommandSize += sizeof(SDMA_PKT_POLL_REGMEM);

	printf("\n\t-----------------------\n");
	printf("\tbuild poll command.\n");
	printf("\tsize = %d.\n", sizeof(SDMA_PKT_POLL_REGMEM));
	for (uint32_t i = 0; i < sizeof(SDMA_PKT_POLL_REGMEM); i++)
	{
		printf("0x%02X, ", *(((unsigned char*)(&poll_cmd)) + i));
		if ((i + 1) % 8 == 0)
			printf("\n");
	}
	printf("\n");
}
static void build_flush_cmd()
{
	SDMA_PKT_HDP_FLUSH hdp_flush_cmd = { 0x8, 0x0, 0x80000000, 0x0, 0x0, 0x0 };

	char * pbuff = (char*)DmaQueueRingBuff + CommandSize;
	memcpy(pbuff, &hdp_flush_cmd, sizeof(SDMA_PKT_HDP_FLUSH));
	CommandSize += sizeof(SDMA_PKT_HDP_FLUSH);

	printf("\n\t-----------------------\n");
	printf("\tbuild flush command.\n");
	printf("\tsize = %d.\n", sizeof(SDMA_PKT_HDP_FLUSH));
	for (uint32_t i = 0; i < sizeof(SDMA_PKT_HDP_FLUSH); i++)
	{
		printf("0x%02X, ", *(((unsigned char*)(&hdp_flush_cmd)) + i));
		if ((i + 1) % 8 == 0)
			printf("\n");
	}
	printf("\n");
}
static void build_atomic_cmd() // BuildAtomicDecrementCommand()
{
	SDMA_PKT_ATOMIC atomic_add_cmd;
	memset(&atomic_add_cmd, 0, sizeof(SDMA_PKT_ATOMIC));

	atomic_add_cmd.HEADER_UNION.op = 10;// SDMA_OP_ATOMIC;
	atomic_add_cmd.HEADER_UNION.operation = 47;// SDMA_ATOMIC_ADD64;
	atomic_add_cmd.ADDR_LO_UNION.addr_31_0 = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(&InitialValue));
	atomic_add_cmd.ADDR_HI_UNION.addr_63_32 = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(&InitialValue) >> 32);
	atomic_add_cmd.SRC_DATA_LO_UNION.src_data_31_0 = 0xffffffff;
	atomic_add_cmd.SRC_DATA_HI_UNION.src_data_63_32 = 0xffffffff;

	char * pbuff = (char*)DmaQueueRingBuff + CommandSize;
	memcpy(pbuff, &atomic_add_cmd, sizeof(SDMA_PKT_ATOMIC));
	CommandSize += sizeof(SDMA_PKT_ATOMIC);

	printf("\n\t-----------------------\n");
	printf("\tbuild atomic add command.\n");
	printf("\tsize = %d.\n", sizeof(SDMA_PKT_ATOMIC));
	for (uint32_t i = 0; i < sizeof(SDMA_PKT_ATOMIC); i++)
	{
		printf("0x%02X, ", *(((unsigned char*)(&atomic_add_cmd)) + i));
		if ((i + 1) % 8 == 0)
			printf("\n");
	}
	printf("\n");
}
static void build_fence_cmd()
{
	SDMA_PKT_FENCE fence_cmd;
	memset(&fence_cmd, 0, sizeof(SDMA_PKT_FENCE));

	uint32_t * mail_box_ptr = (uint32_t *)SdmaEvent->EventData.HWData2;
	fence_cmd.HEADER_UNION.op = 5;// SDMA_OP_FENCE;
	fence_cmd.ADDR_LO_UNION.addr_31_0 = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(mail_box_ptr));
	fence_cmd.ADDR_HI_UNION.addr_63_32 = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(mail_box_ptr) >> 32);
	fence_cmd.DATA_UNION.data = InitialValue;

	char * pbuff = (char*)DmaQueueRingBuff + CommandSize;
	memcpy(pbuff, &fence_cmd, sizeof(SDMA_PKT_FENCE));
	CommandSize += sizeof(SDMA_PKT_FENCE);

	printf("\n\t-----------------------\n");
	printf("\tbuild fence command.\n");
	printf("\tsize = %d.\n", sizeof(SDMA_PKT_FENCE));
	for (uint32_t i = 0; i < sizeof(SDMA_PKT_FENCE); i++)
	{
		printf("0x%02X, ", *(((unsigned char*)(&fence_cmd)) + i));
		if ((i + 1) % 8 == 0)
			printf("\n");
	}
	printf("\n");
}
static void build_trap_cmd()
{
	SDMA_PKT_TRAP trap_cmd;
	memset(&trap_cmd, 0, sizeof(SDMA_PKT_TRAP));

	trap_cmd.HEADER_UNION.op = 6;// SDMA_OP_TRAP;

	char * pbuff = (char*)DmaQueueRingBuff + CommandSize;
	memcpy(pbuff, &trap_cmd, sizeof(SDMA_PKT_TRAP));
	CommandSize += sizeof(SDMA_PKT_TRAP);

	printf("\n\t-----------------------\n");
	printf("\tbuild trap command.\n");
	printf("\tsize = %d.\n", sizeof(SDMA_PKT_TRAP));
	for (uint32_t i = 0; i < sizeof(SDMA_PKT_TRAP); i++)
	{
		printf("0x%02X, ", *(((unsigned char*)(&trap_cmd)) + i));
		if ((i + 1) % 8 == 0)
			printf("\n");
	}
	printf("\n");
}

void InitSdma()
{
	DmaQueueRingBuff = AllocMemoryCPU(SDMA_RINGBUFF_SIZE);
	KmtCreateQueue(KFD_IOC_QUEUE_TYPE_SDMA, DmaQueueRingBuff, SDMA_RINGBUFF_SIZE, &QueueResource);
	ReserveIdx = *reinterpret_cast<uint64_t*>(QueueResource.Queue_read_ptr);
	CommitIdx = *reinterpret_cast<uint64_t*>(QueueResource.Queue_write_ptr);
	DoorBellIdx = *reinterpret_cast<uint64_t*>(QueueResource.QueueDoorBell);
	
	printf("\n=======================\n");
	printf("init blit sdma: create sdma copy queue.\n");
	printf("\tringbuff addr = 0x%016lX.\n", DmaQueueRingBuff);
	printf("\tqueue id      = 0x%016lX.\n", QueueResource.QueueId);
	printf("\tread  pointer = 0x%016lX, Value = %d.\n", QueueResource.QueueRptrValue, ReserveIdx);
	printf("\twrite pointer = 0x%016lX, Value = %d.\n", QueueResource.QueueWptrValue, CommitIdx);
	printf("\tdoorbell addr = 0x%016lX, Value = %d.\n", QueueResource.QueueDoorBell, DoorBellIdx);
	printf("\n");
}
void SdmaCopy(void * src, void * dst, uint32_t copy_size)
{
	CommandSize = 0;
	//build_poll_cmd();
	//build_poll_cmd();
	//build_flush_cmd();
	//build_copy_cmd(src, dst, copy_size);
	build_flush_cmd();
	//build_atomic_cmd();
	//build_fence_cmd();
	//build_trap_cmd();

	uint64_t cur_index, new_index;
	cur_index = ReserveIdx;
	new_index = cur_index + CommandSize;

	printf("\n=======================\n");
	printf("sdma copy.\n");
	printf("\tsrc addr  = 0x%016lX.\n", src);
	printf("\tdst addr  = 0x%016lX.\n", dst);
	printf("\tcopy size = %u.\n", copy_size);
	printf("\tcurrent index = %d.\n", cur_index);
	printf("\tnew     index = %d.\n", new_index);
	printf("\tcommand addr = 0x%016lX.\n", DmaQueueRingBuff);
	printf("\tcommand size = %u.\n", CommandSize);
	printf("\n");

	ReserveIdx = *reinterpret_cast<uint64_t*>(QueueResource.Queue_read_ptr);
	CommitIdx = *reinterpret_cast<uint64_t*>(QueueResource.Queue_write_ptr);
	DoorBellIdx = *reinterpret_cast<uint64_t*>(QueueResource.Queue_DoorBell);
	printf("\tread back value before write:\n");
	printf("\tReserveIndex = %d.\n", ReserveIdx);
	printf("\tCommitIndex  = %d.\n", CommitIdx);
	printf("\tDoorbell     = %d.\n", DoorBellIdx);

	*reinterpret_cast<uint64_t*>(QueueResource.Queue_write_ptr) = new_index;
	*reinterpret_cast<uint64_t*>(QueueResource.Queue_DoorBell) = new_index;

	ReserveIdx = *reinterpret_cast<uint64_t*>(QueueResource.Queue_read_ptr);
	CommitIdx = *reinterpret_cast<uint64_t*>(QueueResource.Queue_write_ptr);
	DoorBellIdx = *reinterpret_cast<uint64_t*>(QueueResource.Queue_DoorBell);
	printf("\tread back value after write:\n");
	printf("\tReserveIndex = %d.\n", ReserveIdx);
	printf("\tCommitIndex  = %d.\n", CommitIdx);
	printf("\tDoorbell     = %d.\n", DoorBellIdx);
}

// ==================================================================
// ==================================================================
void SdmaTest()
{
	MemInit();
	printf("***********************\n");
	printf("* sdma copy test      *\n");
	printf("***********************\n");

	InitSdma();
	SdmaCopy(NULL, NULL, 0);

	/*uint32_t vLen = 1024; // vec_len * sizeof(float) = page_size
	float *h_A, *h_B, *d_A, *d_B;

	h_A = (float*)malloc(vLen * sizeof(float));
	h_B = (float*)malloc(vLen * sizeof(float));
	d_A = (float*)AllocMemoryGPUVM(vLen * sizeof(float));
	d_B = (float*)AllocMemoryGPUVM(vLen * sizeof(float));

	for (uint32_t i = 0; i < vLen; i++)
		h_A[i] = 1.0f * i;

	//SdmaEvent = CreateSignal();
	InitSdma();
	SdmaCopy(h_A, d_A, vLen * sizeof(float));
	sleep(1000);

	FreeMemoryGPUVM(d_A);
	FreeMemoryGPUVM(d_B);
	free(h_A);
	free(h_B);*/

	printf("\n");
	MemDeInit();
}
