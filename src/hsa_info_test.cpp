#include <string>
#include <iostream>
#include <vector>

#include "hsa_test.h"

using namespace std;

int gNodeNum; int gNodeId = 0;
int gGpuNodeNum = 0; std::vector<uint32_t> gGpuNodeIds;
int gLinkNum; int gLinkId;
int gCacheNum; int gCacheId;

using namespace std;

string iotype_to_string(HSA_IOLINKTYPE t)
{
	switch (t)
	{
	case HSA_IOLINKTYPE_UNDEFINED: return "UNDEFINED";
	case HSA_IOLINKTYPE_HYPERTRANSPORT: return "HYPERTRANSPORT";
	case HSA_IOLINKTYPE_PCIEXPRESS: return "PCIEXPRESS";
	case HSA_IOLINKTYPE_AMBA: return "AMBA";
	case HSA_IOLINKTYPE_MIPI: return "MIPI";
	case HSA_IOLINK_TYPE_QPI_1_1: return "QPI_1_1";
	case HSA_IOLINK_TYPE_RESERVED1: return "RESERVED1";
	case HSA_IOLINK_TYPE_RESERVED2: return "RESERVED2";
	case HSA_IOLINK_TYPE_RAPID_IO: return "RAPID_IO";
	case HSA_IOLINK_TYPE_INFINIBAND: return "INFINIBAND";
	case HSA_IOLINK_TYPE_RESERVED3: return "RESERVED3";
	case HSA_IOLINKTYPE_OTHER: return "OTHER";
	case HSA_IOLINKTYPE_NUMIOLINKTYPES: return "NUMIOLINKTYPES";
	}

}
void hsa_link_info(int node_id, int num_link)
{
	vector<HsaIoLinkProperties> link_props(num_link);
	hsaKmtGetNodeIoLinkProperties(node_id, num_link, &link_props[0]);

	for (HsaIoLinkProperties link : link_props)
	{
		printf("\t\t IoLinkType: %s\n", iotype_to_string(link.IoLinkType).c_str());
		printf("\t\t bus version: %d.%d\n", link.VersionMajor,link.VersionMinor);
		printf("\t\t node direction: %d --> %d\n", link.NodeFrom, link.NodeTo);
		printf("\t\t numa distance: %d\n", link.Weight);
		printf("\t\t MinimumLatency: %d(ns)\n", link.MinimumLatency);
		printf("\t\t MaximumLatency: %d(ns)\n", link.MaximumLatency);
		printf("\t\t MinimumBandwidth: %d(MB/s)\n", link.MinimumBandwidth);
		printf("\t\t MaximumBandwidth: %d(MB/s)\n", link.MaximumBandwidth);
		printf("\t\t recommended transfer size: %d\n", link.RecTransferSize);

		printf("\t\t Capability:\n");
		printf("\t\t Override: %c\n", link.Flags.ui32.Override ? 'Y' : 'F');
		printf("\t\t NonCoherent: %c\n", link.Flags.ui32.NonCoherent ? 'Y' : 'F');
		printf("\t\t NoAtomics32bit: %c\n", link.Flags.ui32.NoAtomics32bit ? 'Y' : 'F');
		printf("\t\t NoAtomics64bit: %c\n", link.Flags.ui32.NoAtomics64bit ? 'Y' : 'F');
		printf("\t\t NoPeerToPeerDMA: %c\n", link.Flags.ui32.NoPeerToPeerDMA ? 'Y' : 'F');
		printf("\n");
	}
}

string heaptype_to_string(HSA_HEAPTYPE t)
{
	switch (t)
	{
	case HSA_HEAPTYPE_SYSTEM: return "SYSTEM";
	case HSA_HEAPTYPE_FRAME_BUFFER_PUBLIC: return "PUBLIC";
	case HSA_HEAPTYPE_FRAME_BUFFER_PRIVATE: return "PRIVATE";
	case HSA_HEAPTYPE_GPU_GDS: return "GDS";
	case HSA_HEAPTYPE_GPU_LDS: return "LDS";
	case HSA_HEAPTYPE_GPU_SCRATCH: return "SCRATCH";
	case HSA_HEAPTYPE_DEVICE_SVM: return "SVM";
	}
}
void hsa_mem_info(int node_id, int num_mem)
{
	vector<HsaMemoryProperties> mem_props(num_mem);
	hsaKmtGetNodeMemoryProperties(node_id, num_mem, &mem_props[0]);

	for (HsaMemoryProperties mem : mem_props)
	{
		printf("\t\t heap type: %s\n", heaptype_to_string(mem.HeapType).c_str());
		printf("\t\t size: %.3f(MB)\n", mem.SizeInBytes / 1024.0 / 1024.0);
		printf("\t\t width: %d\n", mem.Width);
		printf("\t\t clock: %.3f(MHz)\n", mem.MemoryClockMax / 1000.0 / 1000.0);
		printf("\t\t virtual address: 0x%08X\n", mem.VirtualBaseAddress);

		printf("\t\t Capability:\n");
		printf("\t\t HotPluggable: %c\n", mem.Flags.ui32.HotPluggable ? 'Y' : 'F');
		printf("\t\t NonVolatile: %c\n", mem.Flags.ui32.NonVolatile ? 'Y' : 'F');
		printf("\n");
	}
}

string cachetype_to_string(HsaCacheType t)
{
	if (t.ui32.Data) return "Data";
	if (t.ui32.Instruction) return "Instruction";
	if (t.ui32.CPU) return "CPU";
	if (t.ui32.HSACU) return "HSACU";
}
void hsa_cache_info(int node_id, int cache_mem, int processorId)
{
	vector<HsaCacheProperties> cache_props(cache_mem);
	hsaKmtGetNodeCacheProperties(node_id, processorId, cache_mem, &cache_props[0]);

	for (HsaCacheProperties cache : cache_props)
	{
		printf("\t\t type: %s\n", cachetype_to_string(cache.CacheType).c_str());
		printf("\t\t processor id: 0x%08X\n", cache.ProcessorIdLow);
		printf("\t\t level: %d\n", cache.CacheLevel);
		printf("\t\t size: %d(KB)\n", cache.CacheSize/1024);
		printf("\t\t cacheline size: %d(byte)\n", cache.CacheLineSize);
		printf("\t\t cacheline number per tag: %d\n", cache.CacheLinesPerTag);
		printf("\t\t associativity: %d\n", cache.CacheAssociativity);
		printf("\t\t latency: %d(ns)\n", cache.CacheLatency);
		printf("\n");
	}
}

void hsa_node_info(int num_node)
{
	int fComputeIdLo;
	HsaNodeProperties node;

	for (int node_id = 0; node_id < num_node; node_id++)
	{
		hsaKmtGetNodeProperties(node_id, &node);

		printf("\t ======================\n");
		wcout << "\t " << node.MarketingName << endl;
		printf("\t CAL Name: %s\n", node.AMDName);
		printf("\t VendorId: 0x%04X\n", node.VendorId);
		printf("\t DeviceId: 0x%04X\n", node.DeviceId);
		printf("\t hsa engine id: %d.%d.%d.%d\n",
			node.EngineId.ui32.uCode, node.EngineId.ui32.Major,
			node.EngineId.ui32.Minor, node.EngineId.ui32.Stepping);
		printf("\t hsa engine version: %d.%d.%d\n",
			node.uCodeEngineVersions.uCodeSDMA,
			node.uCodeEngineVersions.uCodeRes, node.uCodeEngineVersions.Reserved);
		printf("\t DRM id: 0x%08X\n", node.DrmRenderMinor);
		printf("\t bus id: 0x%08X\n", node.LocationId);

		printf("\t*CPU clocks: %.3f(GHz)\n", node.MaxEngineClockMhzCCompute / 1000.0);
		printf("\t*GPU clocks: %.3f(GHz)\n", node.MaxEngineClockMhzFCompute / 1000.0);

		printf("\t CComputeIdLo: 0x%08X\n", node.CComputeIdLo);
		printf("\t FComputeIdLo: 0x%08X\n", node.FComputeIdLo);
		printf("\t*cpu core number: %d\n", node.NumCPUCores);
		printf("\t*gpu simd number: %d\n", node.NumFComputeCores);
		printf("\t shader engines number: %d\n", node.NumShaderBanks);
		printf("\t simd arrays number per engine: %d\n", node.NumArrays);
		printf("\t cu number per engines: %d\n", node.NumCUPerArray);
		printf("\t simd number per cu: %d\n", node.NumSIMDPerCU);

		printf("\t max wave number per simd: %d\n", node.MaxWavesPerSIMD);
		printf("\t wave size: %d\n", node.WaveFrontSize);

		printf("\t MaxSlotsScratchCU number: %d\n", node.MaxSlotsScratchCU);
		printf("\t*local memory size: %.3f(GB)\n", node.LocalMemSize / 1024.0 / 1024.0 / 1024.0);
		printf("\t*LDS: %d(KB)\n", node.LDSSizeInKB);
		printf("\t GDS: %d(KB)\n", node.GDSSizeInKB);

		printf("\t Capability:\n");
		printf("\t HotPluggable: %c\n", node.Capability.ui32.HotPluggable ? 'Y' : 'F');
		printf("\t HSAMMUPresent: %c\n", node.Capability.ui32.HSAMMUPresent ? 'Y' : 'F');
		printf("\t SharedWithGraphics: %c\n", node.Capability.ui32.SharedWithGraphics ? 'Y' : 'F');
		printf("\t QueueSizePowerOfTwo: %c\n", node.Capability.ui32.QueueSizePowerOfTwo ? 'Y' : 'F');
		printf("\t QueueSize32bit: %c\n", node.Capability.ui32.QueueSize32bit ? 'Y' : 'F');
		printf("\t QueueIdleEvent: %c\n", node.Capability.ui32.QueueIdleEvent ? 'Y' : 'F');
		printf("\t VALimit: %c\n", node.Capability.ui32.VALimit ? 'Y' : 'F');
		printf("\t WatchPointsSupported: %c\n", node.Capability.ui32.WatchPointsSupported ? 'Y' : 'F');
		printf("\t WatchPointsTotalBits: %d\n", node.Capability.ui32.WatchPointsTotalBits);
		printf("\t DoorbellType: %d\n", node.Capability.ui32.DoorbellType);
		printf("\t AQLQueueDoubleMap: %c\n", node.Capability.ui32.AQLQueueDoubleMap ? 'Y' : 'F');
		printf("\t link number: %d\n", node.NumIOLinks);
		printf("\t memory bank number: %d\n", node.NumMemoryBanks);
		printf("\t cache number: %d\n", node.NumCaches);

		HsaClockCounters t;
		hsaKmtGetClockCounters(node_id, &t);
		printf("\t CPU clock counter = %ld\n", t.CPUClockCounter);
		printf("\t GPU clock counter = %ld\n", t.GPUClockCounter);
		printf("\t system clock counter = %ld\n", t.SystemClockCounter);
		printf("\t system clock freq = %.3f(GHz)\n", t.SystemClockFrequencyHz / 1000.0 / 1000.0 / 1000.0);
		printf("\n");

		if (t.GPUClockCounter != 0)
		{
			gGpuNodeNum++;
			gGpuNodeIds.push_back(node_id);
		}

		if (node_id == gNodeId)
		{
			gLinkNum = node.NumIOLinks;
			gMemNum = node.NumMemoryBanks;
			gCacheNum = node.NumCaches;
			fComputeIdLo = node.FComputeIdLo;
		}
	}

	printf("======================\n");
	printf("sellceted node %d:\n", gNodeId);
	printf("\t ---------------------\n");
	printf("\t link number: %d\n", gLinkNum);
	hsa_link_info(gNodeId, gLinkNum);
	printf("\t ---------------------\n");
	printf("\t memory bank number = %d.\n", gMemNum);
	hsa_mem_info(gNodeId, gMemNum);
	printf("\t ---------------------\n");
	printf("\t cache number = %d.\n", gCacheNum);
	//hsa_cache_info(gNodeId, gCacheNum, fComputeIdLo);
	printf("======================\n");
	printf("\n");
}

void hsa_sys_info()
{
#if defined(__LP64__) || defined(_M_X64)
	printf("hsa large mode, max queue number = 128\n");
#endif

	HsaVersionInfo info;
	hsaKmtGetVersion(&info);
	printf("kmt version: %d.%d\n\n", info.KernelInterfaceMajorVersion, info.KernelInterfaceMinorVersion);

	HsaSystemProperties sys_props;
	hsaKmtReleaseSystemProperties();
	hsaKmtAcquireSystemProperties(&sys_props);
	printf("kmt node number: %d\n", sys_props.NumNodes);
	printf("kmt OEM ID: 0x%08X\n", sys_props.PlatformOem);
	printf("kmt platform ID: 0x%08X\n", sys_props.PlatformId);
	printf("kmt platform revision: %d\n\n", sys_props.PlatformRev);

	hsa_node_info(sys_props.NumNodes);
}

void hsa_info_test()
{
	hsa_sys_info();
}
