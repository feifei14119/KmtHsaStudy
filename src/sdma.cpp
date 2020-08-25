#include "kmthsa.h"

#define MAX_SDMA_COPY_SIZE (0x3fffe0) // 4M - 32
#define SDMA_RINGBUFF_SIZE (1024 * 1024)

static uint32_t CommandSize = 0;
static void * DmaQueueRingBuff; // ring buffer address for sdma copy queue
static HsaQueueResource QueueResource;

typedef struct SDMA_PKT_WRITE_UNTILED_TAG
{
	union
	{
		struct
		{
			unsigned int op : 8;
			unsigned int sub_op : 8;
			unsigned int reserved_0 : 16;
		};
		unsigned int DW_0_DATA;
	} HEADER_UNION;

	union
	{
		struct
		{
			unsigned int dst_addr_31_0 : 32;
		};
		unsigned int DW_1_DATA;
	} DST_ADDR_LO_UNION;

	union
	{
		struct
		{
			unsigned int dst_addr_63_32 : 32;
		};
		unsigned int DW_2_DATA;
	} DST_ADDR_HI_UNION;

	union
	{
		struct
		{
			unsigned int count : 22;
			unsigned int reserved_0 : 2;
			unsigned int sw : 2;
			unsigned int reserved_1 : 6;
		};
		unsigned int DW_3_DATA;
	} DW_3_UNION;

	union
	{
		struct
		{
			unsigned int data0 : 32;
		};
		unsigned int DW_4_DATA;
	} DATA0_UNION;
} SDMA_PKT_WRITE_UNTILED;
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
typedef struct SDMA_PKT_HDP_FLUSH_TAG 
{
	unsigned int DW_0_DATA;
	unsigned int DW_1_DATA;
	unsigned int DW_2_DATA;
	unsigned int DW_3_DATA;
	unsigned int DW_4_DATA;
	unsigned int DW_5_DATA;

	// Version of gfx9 sDMA microcode introducing SDMA_PKT_HDP_FLUSH
	static const uint16_t kMinVersion_ = 0x1A5;
} SDMA_PKT_HDP_FLUSH;
typedef struct SDMA_PKT_POLL_REGMEM_TAG
{
	union
	{
		struct
		{
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

	union
	{
		struct
		{
			unsigned int addr_31_0 : 32;
		};
		unsigned int DW_1_DATA;
	} ADDR_LO_UNION;

	union
	{
		struct
		{
			unsigned int addr_63_32 : 32;
		};
		unsigned int DW_2_DATA;
	} ADDR_HI_UNION;

	union
	{
		struct
		{
			unsigned int value : 32;
		};
		unsigned int DW_3_DATA;
	} VALUE_UNION;

	union
	{
		struct
		{
			unsigned int mask : 32;
		};
		unsigned int DW_4_DATA;
	} MASK_UNION;

	union
	{
		struct
		{
			unsigned int interval : 16;
			unsigned int retry_count : 12;
			unsigned int reserved_0 : 4;
		};
		unsigned int DW_5_DATA;
	} DW5_UNION;
} SDMA_PKT_POLL_REGMEM;
typedef struct SDMA_PKT_FENCE_TAG
{
	union
	{
		struct
		{
			unsigned int op : 8;
			unsigned int sub_op : 8;
			unsigned int reserved_0 : 16;
		};
		unsigned int DW_0_DATA;
	} HEADER_UNION;

	union
	{
		struct
		{
			unsigned int addr_31_0 : 32;
		};
		unsigned int DW_1_DATA;
	} ADDR_LO_UNION;

	union
	{
		struct
		{
			unsigned int addr_63_32 : 32;
		};
		unsigned int DW_2_DATA;
	} ADDR_HI_UNION;

	union
	{
		struct
		{
			unsigned int data : 32;
		};
		unsigned int DW_3_DATA;
	} DATA_UNION;
} SDMA_PKT_FENCE;
typedef struct SDMA_PKT_TRAP_TAG
{
	union
	{
		struct
		{
			unsigned int op : 8;
			unsigned int sub_op : 8;
			unsigned int reserved_0 : 16;
		};
		unsigned int DW_0_DATA;
	} HEADER_UNION;

	union
	{
		struct
		{
			unsigned int int_ctx : 28;
			unsigned int reserved_1 : 4;
		};
		unsigned int DW_1_DATA;
	} INT_CONTEXT_UNION;
} SDMA_PKT_TRAP;

// ==================================================================

static void build_write_cmd(void * addr, uint32_t data)
{
	SDMA_PKT_WRITE_UNTILED write_untiled_cmd;
	memset(&write_untiled_cmd, 0, sizeof(SDMA_PKT_WRITE_UNTILED));
	
	write_untiled_cmd.HEADER_UNION.op = 2;// SDMA_OP_WRITE;
	write_untiled_cmd.HEADER_UNION.sub_op = 0;// SDMA_SUBOP_WRITE_LINEAR;	
	write_untiled_cmd.DST_ADDR_LO_UNION.DW_1_DATA = static_cast<unsigned int>((uint64_t)addr);
	write_untiled_cmd.DST_ADDR_HI_UNION.DW_2_DATA = static_cast<unsigned int>((uint64_t)addr >> 32);
	write_untiled_cmd.DW_3_UNION.count = 0;
	write_untiled_cmd.DATA0_UNION.DW_4_DATA = data;

	char * pbuff = (char*)DmaQueueRingBuff + CommandSize;
	memcpy(pbuff, &write_untiled_cmd, sizeof(SDMA_PKT_WRITE_UNTILED));
	CommandSize += sizeof(SDMA_PKT_WRITE_UNTILED);

	printf("\n\t-----------------------\n");
	printf("\tbuild write untiled command.\n");
	printf("\tsize = %d.\n", sizeof(SDMA_PKT_WRITE_UNTILED));
	for (uint32_t i = 0; i < sizeof(SDMA_PKT_WRITE_UNTILED); i++)
	{
		printf("0x%02X, ", *(((unsigned char*)(&write_untiled_cmd)) + i));
		if ((i + 1) % 8 == 0)
			printf("\n");
	}
	printf("\n\n");
}
static void build_copy_cmd(void * dst, void * src, uint32_t copy_size)
{
	assert(copy_size < MAX_SDMA_COPY_SIZE);

	SDMA_PKT_COPY_LINEAR dma_copy_cmd;
	memset(&dma_copy_cmd, 0, sizeof(SDMA_PKT_COPY_LINEAR));

	dma_copy_cmd.HEADER_UNION.op = 1;// SDMA_OP_COPY;
	dma_copy_cmd.HEADER_UNION.sub_op = 0;// SDMA_SUBOP_COPY_LINEAR;
	dma_copy_cmd.COUNT_UNION.count = copy_size - 1;
	dma_copy_cmd.SRC_ADDR_LO_UNION.src_addr_31_0 = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(src));
	dma_copy_cmd.SRC_ADDR_HI_UNION.src_addr_63_32 = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(src) >> 32);
	dma_copy_cmd.DST_ADDR_LO_UNION.dst_addr_31_0 = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(dst));
	dma_copy_cmd.DST_ADDR_HI_UNION.dst_addr_63_32 = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(dst) >> 32);

	char * pbuff = (char*)DmaQueueRingBuff + CommandSize;
	memcpy(pbuff, &dma_copy_cmd, sizeof(SDMA_PKT_COPY_LINEAR));
	CommandSize += sizeof(SDMA_PKT_COPY_LINEAR);

	printf("\n\t-----------------------\n");
	printf("\tbuild copy command.\n");
	printf("\tsize = %d(DWORD).\n", sizeof(SDMA_PKT_COPY_LINEAR) / 4);
	for (uint32_t i = 0; i < sizeof(SDMA_PKT_COPY_LINEAR) / 4; i++)
	{
		printf("0x%08X, ", *(((unsigned int*)(&dma_copy_cmd)) + i));
		if ((i + 1) % 4 == 0)
			printf("\n");
	}
	printf("\n\n");
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
/*static void build_poll_cmd()
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
}*/
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

void hsaInitSdma()
{
	printf("\n==============================================\n");
	printf("init blit sdma: create sdma copy queue.\n");

	CommandSize = 0;
	DmaQueueRingBuff = HsaAllocCPU(SDMA_RINGBUFF_SIZE);
	KmtCreateQueue(KFD_IOC_QUEUE_TYPE_SDMA, DmaQueueRingBuff, SDMA_RINGBUFF_SIZE, &QueueResource);

	printf("\tqueue id      = 0x%016lX.\n", QueueResource.QueueId);
	printf("\twrite pointer = 0x%016lX, Value = %d.\n", QueueResource.QueueWptrValue, *QueueResource.Queue_write_ptr);
	printf("\tread  pointer = 0x%016lX, Value = %d.\n", QueueResource.QueueRptrValue, *QueueResource.Queue_read_ptr);
	printf("\tringbuff addr = 0x%016lX.\n", DmaQueueRingBuff);
	printf("\tdoorbell addr = 0x%016lX.\n", QueueResource.QueueDoorBell);
	printf("==============================================\n");
	printf("\n");
}
void hsaDeInitSdma()
{
	KmtDestroyQueue(QueueResource.QueueId);
}

void HsaSdmaWrite(void * dst, uint32_t data)
{
	printf("=============================\n");
	printf("sdma write:\n");

	build_write_cmd(dst, data);

	uint64_t cur_index, new_index;
	cur_index = *QueueResource.Queue_read_ptr;
	new_index = cur_index + CommandSize;

	*(QueueResource.Queue_write_ptr_aql) = new_index;
	*(QueueResource.Queue_DoorBell_aql) = new_index;
	while (*QueueResource.Queue_write_ptr != *QueueResource.Queue_read_ptr)
	{
		usleep(1000);
	}

	printf("\tread  pointer = 0x%016lX, Value = %d.\n", QueueResource.QueueRptrValue, *(volatile unsigned int *)QueueResource.Queue_read_ptr);
	printf("\twrite pointer = 0x%016lX, Value = %d.\n", QueueResource.QueueWptrValue, *(volatile unsigned int *)QueueResource.Queue_write_ptr);
	printf("=============================\n");
	printf("\n");
}
void HsaSdmaCopy(void * dst, void * src, uint32_t copy_size)
{
	printf("=============================\n");
	printf("sdma copy:\n");
	build_copy_cmd(dst, src, copy_size);

	uint64_t cur_index, new_index;
	cur_index = *QueueResource.Queue_read_ptr;
	new_index = cur_index + CommandSize;

	*(QueueResource.Queue_write_ptr_aql) = new_index;
	*(QueueResource.Queue_DoorBell_aql) = new_index;
	while (*QueueResource.Queue_write_ptr != *QueueResource.Queue_read_ptr)
	{
		usleep(1000);
	}

	printf("\tread  pointer = 0x%016lX, Value = %d.\n", QueueResource.QueueRptrValue, *(volatile unsigned int *)QueueResource.Queue_read_ptr);
	printf("\twrite pointer = 0x%016lX, Value = %d.\n", QueueResource.QueueWptrValue, *(volatile unsigned int *)QueueResource.Queue_write_ptr);
	printf("=============================\n");
	printf("\n");
}

// ==================================================================

void RunSdmaTest()
{
	uint32_t in_val  = 0x01234567;
	uint32_t out_val = 0;
	
	uint32_t * h_A;
	h_A = (uint32_t*)HsaAllocCPU(4 * sizeof(float));

	HsaSdmaWrite(h_A, in_val);
	out_val = *h_A;

	printf("HsaSdmaWrite out value = 0x%08X\n", out_val);
	if (in_val == out_val)
	{
		printf("PASS\n");
	}
	else
	{
		printf("Failed\n");
	}

	HsaFreeMem(h_A);
}
