#include <string>
#include <iostream>
#include <vector>
#include <sys/mman.h>

#include "kmt_test.h"

using namespace std;

void ff_kmt_alloc_host_cpu(void ** alloc_addr, size_t alloc_size, HsaMemFlags mem_flag)
{
	int mmap_prot = PROT_READ;
	
	if (mem_flag.ui32.ExecuteAccess)
		mmap_prot |= PROT_EXEC;

	if (!mem_flag.ui32.ReadOnly)
		mmap_prot |= PROT_WRITE;

	void *mem_addr = NULL;
	mem_addr = mmap(NULL, alloc_size, mmap_prot, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);

	*alloc_addr = mem_addr;

	printf("kmt alloc: %d(B) at 0x%X.\n", alloc_size, *alloc_addr);
	printf("kmt mmap_port: ");
	if(mmap_prot & PROT_READ)		printf("PROT_READ");
	if (mmap_prot & PROT_EXEC)		printf(" | PROT_EXEC");
	if (mmap_prot & PROT_WRITE)		printf(" | PROT_WRITE");
	printf("\n");
}
void ff_kmt_free_host_cpu(void * mem_addr, size_t mem_size)
{
	munmap(mem_addr, mem_size);
	printf("kmt free host cpu memory: %d(B) at 0x%X.\n", mem_size, mem_addr);
	printf("\n");
}

void kmt_mem_test()
{
	printf("***********************\n");
	printf("* hsa memory test     *\n");
	printf("***********************\n");

	
}
