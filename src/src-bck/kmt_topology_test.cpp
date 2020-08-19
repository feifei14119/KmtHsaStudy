#include <string>
#include <iostream>
#include <vector>
#include <sys/mman.h>
#include <errno.h>

#include "kmt_test.h"

static int testNodeId = 1;
static int testMemId = 0;
static int testCacheId = 0;
static int testLinkId = 0;
static int gMemBankNum;
static int gCacheNum;
static int gLinkNum;
static node_props_t *g_props;
#define NUM_OF_DGPU_HEAPS 3

// ==================================================================
// ==================================================================
void topology_take_snapshot()
{
	printf("=======================\n");
	printf("create topology info for %d nodes.\n", testNodeId);

	g_props = (node_props_t*)calloc(gKmtNodeNum * sizeof(node_props_t), 1);

	gMemBankNum = readIntKey(KFD_NODE_PROP(testNodeId), "mem_banks_count");
	printf("memory bank number = %d.\n", gMemBankNum);
	gCacheNum = readIntKey(KFD_NODE_PROP(testNodeId), "caches_count");
	printf("cache number = %d.\n", gCacheNum);
	gLinkNum = readIntKey(KFD_NODE_PROP(testNodeId), "io_links_count");
	printf("link number = %d.\n", gLinkNum);
	printf("\n");

}
void get_mem_props()
{
	printf("=======================\n");
	printf("create memory info.\n");

	gMemBankNum += NUM_OF_DGPU_HEAPS; // lds, gpuvm, scratch, mmio
	g_props->mem = (HsaMemoryProperties*)calloc(gMemBankNum * sizeof(HsaMemoryProperties), 1);

	int heap_type = readIntKey(KFD_NODE_MEM_PROP(testNodeId, testMemId), "heap_type");
	printf("heap_type = %d.\n", heap_type);
	uint64_t size_in_bytes = readIntKey(KFD_NODE_MEM_PROP(testNodeId, testMemId), "size_in_bytes");
	printf("size_in_bytes = %lld.\n", size_in_bytes);
	int flags = readIntKey(KFD_NODE_MEM_PROP(testNodeId, testMemId), "flags");
	printf("flags = %d.\n", flags);
	int width = readIntKey(KFD_NODE_MEM_PROP(testNodeId, testMemId), "width");
	printf("width = %d.\n", width);
	int mem_clk_max = readIntKey(KFD_NODE_MEM_PROP(testNodeId, testMemId), "mem_clk_max");
	printf("mem_clk_max = %d.\n", mem_clk_max);


	printf("\n"); 
}
void get_cache_props()
{
	printf("=======================\n");
	printf("create cache info.\n");

	g_props->cache = (HsaCacheProperties*)calloc(gCacheNum * sizeof(HsaCacheProperties), 1);

	uint32_t processor_id_low = readIntKey(KFD_NODE_CACHE_PROP(testNodeId, testCacheId), "processor_id_low");
	printf("processor_id_low = %d.\n", processor_id_low);
	uint32_t level = readIntKey(KFD_NODE_CACHE_PROP(testNodeId, testCacheId), "level");
	printf("level = %d.\n", level);
	uint64_t size = readIntKey(KFD_NODE_CACHE_PROP(testNodeId, testCacheId), "size");
	printf("size = %lld.\n", size);
	int cache_line_size = readIntKey(KFD_NODE_CACHE_PROP(testNodeId, testCacheId), "cache_line_size");
	printf("cache_line_size = %d.\n", cache_line_size);
	int cache_lines_per_tag = readIntKey(KFD_NODE_CACHE_PROP(testNodeId, testCacheId), "cache_lines_per_tag");
	printf("cache_lines_per_tag = %d.\n", cache_lines_per_tag);
	int association = readIntKey(KFD_NODE_CACHE_PROP(testNodeId, testCacheId), "association");
	printf("association = %d.\n", association);
	int latency = readIntKey(KFD_NODE_CACHE_PROP(testNodeId, testCacheId), "latency");
	printf("latency = %d.\n", latency);
	int type = readIntKey(KFD_NODE_CACHE_PROP(testNodeId, testCacheId), "type");
	printf("type = %d.\n", type);
	printf("\n");
}
void get_link_props()
{
	printf("=======================\n");
	printf("create link info.\n");

	g_props->link = (HsaIoLinkProperties*)calloc(gCacheNum * sizeof(HsaIoLinkProperties), 1);

	uint32_t type = readIntKey(KFD_NODE_LINK_PROP(testNodeId, testLinkId), "type");
	printf("type = %d.\n", type);
	uint32_t version_major = readIntKey(KFD_NODE_LINK_PROP(testNodeId, testLinkId), "version_major");
	printf("version_major = %d.\n", version_major);
	uint32_t version_minor = readIntKey(KFD_NODE_LINK_PROP(testNodeId, testLinkId), "version_minor");
	printf("version_minor = %d.\n", version_minor);
	uint32_t node_from = readIntKey(KFD_NODE_LINK_PROP(testNodeId, testLinkId), "node_from");
	printf("node_from = %d.\n", node_from);
	uint32_t node_to = readIntKey(KFD_NODE_LINK_PROP(testNodeId, testLinkId), "node_to");
	printf("node_to = %d.\n", node_to);
	uint32_t weight = readIntKey(KFD_NODE_LINK_PROP(testNodeId, testLinkId), "weight");
	printf("weight = %d.\n", weight);
	uint32_t min_latency = readIntKey(KFD_NODE_LINK_PROP(testNodeId, testLinkId), "min_latency");
	printf("min_latency = %d.\n", min_latency);
	uint32_t max_latency = readIntKey(KFD_NODE_LINK_PROP(testNodeId, testLinkId), "max_latency");
	printf("max_latency = %d.\n", max_latency);
	uint32_t min_bandwidth = readIntKey(KFD_NODE_LINK_PROP(testNodeId, testLinkId), "min_bandwidth");
	printf("min_bandwidth = %d.\n", min_bandwidth);
	uint32_t max_bandwidth = readIntKey(KFD_NODE_LINK_PROP(testNodeId, testLinkId), "max_bandwidth");
	printf("max_bandwidth = %d.\n", max_bandwidth);
	uint32_t recommended_transfer_size = readIntKey(KFD_NODE_LINK_PROP(testNodeId, testLinkId), "recommended_transfer_size");
	printf("recommended_transfer_size = %d.\n", recommended_transfer_size);
	uint32_t flags = readIntKey(KFD_NODE_LINK_PROP(testNodeId, testLinkId), "flags");
	printf("flags = %d.\n", flags);
	printf("\n");
}

// ==================================================================
// ==================================================================
void kmt_topology_test()
{
	printf("***********************\n");
	printf("* kmt topology test    \n");
	printf("***********************\n");

	topology_take_snapshot();
	get_mem_props();
	get_cache_props();
	get_link_props();

	printf("\n");
}
