#include <string>
#include <iostream>
#include <vector>
#include <sys/mman.h>
#include <errno.h>

#include "kmt_test.h"

struct queue 
{
	uint32_t queue_id;
	uint64_t wptr;
	uint64_t rptr;
	void *eop_buffer;
	void *ctx_save_restore;
	uint32_t ctx_save_restore_size;
	uint32_t ctl_stack_size;
	const struct device_info *dev_info;
	bool use_ats;
	/* This queue structure is allocated from GPU with page aligned size
	 * but only small bytes are used. We use the extra space in the end for
	 * cu_mask bits array.
	 */
	uint32_t cu_mask_count; /* in bits */
	uint32_t cu_mask[0];
};

static unsigned int num_doorbells;
static struct process_doorbells *doorbells;

// ==================================================================
// ==================================================================
void init_process_doorbells()
{
	printf("=======================\n");
	printf("init doorbells for %d nodes.\n", gKmtNodeNum);

	unsigned int i;

	/* doorbells[] is accessed using Topology NodeId. This means doorbells[0],
	 * which corresponds to CPU only Node, might not be used
	 */
	doorbells = (process_doorbells*)malloc(gKmtNodeNum * sizeof(struct process_doorbells));

	for (i = 0; i < gKmtNodeNum; i++)
	{
		doorbells[i].use_gpuvm = false;
		doorbells[i].size = 0;
		doorbells[i].mapping = NULL;
		pthread_mutex_init(&doorbells[i].mutex, NULL);
	}

	num_doorbells = gKmtNodeNum;
}

void create_queue()
{
	printf("=======================\n");
	printf("create queue.\n");
}

// ==================================================================
// ==================================================================
void kmt_queue_test()
{
	printf("***********************\n");
	printf("* kmt queue test      *\n");
	printf("***********************\n");

	init_process_doorbells();
	create_queue();
	printf("\n");
}
