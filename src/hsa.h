#pragma once

#include <string>
#include <iostream>
#include <vector>

#include <stdio.h>
#include <strings.h>
#include <string.h> 
#include <iostream>
#include <vector>
#include <sstream>

#include <errno.h>
#include <assert.h>

#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/mman.h>

using namespace std;

// ==================================================================
// user API
// ==================================================================
extern void HsaInit();
extern void HsaDeInit();

extern void * HsaAllocCPU(uint64_t memSize);
extern void * HsaAllocGPU(uint64_t memSize);
extern void HsaFreeMem(void * memAddr);

extern void HsaInitSdma();
extern void HsaDeInitSdma();
extern void HsaSdmaWrite(void * dst, uint32_t data);
extern void HsaSdmaCopy(void * dst, void * src, uint32_t copy_size);

