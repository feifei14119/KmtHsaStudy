#pragma once

#include "stdio.h"
#include "hsakmt.h"

#define hsa_test_open() do{\
	HSAKMT_STATUS kmt_status;\
	kmt_status = hsaKmtOpenKFD();\
	if (kmt_status != HSAKMT_STATUS_SUCCESS)\
		printf("failed to open hsa! err = %d\n", kmt_status);\
}while(0)

#define hsa_test_close() do{\
	HSAKMT_STATUS kmt_status;\
	kmt_status = hsaKmtCloseKFD();\
	if (kmt_status != HSAKMT_STATUS_SUCCESS)\
		printf("failed to close hsa! err = %d\n", kmt_status);\
}while(0)

extern int gNodeNum; extern int gNodeId;
extern int gGpuNodeNum; 
extern std::vector<uint32_t> gGpuNodeIds;
extern int gLinkNum; extern int gLinkId;
extern int gMemNum; extern int gMemId;
extern int gCacheNum; extern int gCacheId;

extern void hsa_info_test();
extern void hsa_queue_test();
extern void hsa_mem_test();
