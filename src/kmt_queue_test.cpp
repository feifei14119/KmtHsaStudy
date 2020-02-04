#include <string>
#include <iostream>
#include <vector>
#include <sys/mman.h>
#include <errno.h>

#include "kmt_test.h"


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

// ==================================================================
// ==================================================================
void kmt_queue_test()
{
	printf("***********************\n");
	printf("* kmt queue test      *\n");
	printf("***********************\n");

	init_process_doorbells();
}
