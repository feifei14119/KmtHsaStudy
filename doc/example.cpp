#include "dispatch.hpp"

using namespace amd::dispatch;

class AsmKernelDispatch : public Dispatch 
{
private:
	Buffer* out;

public:
	AsmKernelDispatch(int argc, const char **argv) : Dispatch(argc, argv) { }

	bool SetupCodeObject() override 
	{
		return LoadCodeObjectFromFile("asm-kernel.co");
	}

	bool Setup() override 
	{
		if (!AllocateKernarg(1024)) { return false; }
		out = AllocateBuffer(1024);
		Kernarg(out);
		SetGridSize(1);
		SetWorkgroupSize(1);
		return true;
	}

	bool Verify() override 
	{
		if (!CopyFrom(out))
		{
			output << "Error: failed to copy from local" << std::endl;
			return false;
		}
		if (((const float*)out->SystemPtr())[0] != 3.14159f)
		{
			output << "Error: validation failed: got " << (((const float*) out->SystemPtr())[0]) << " expected " << 3.14159 << std::endl;
			return false;
		}
		return true;
	}
};

int main(int argc, const char** argv)
{
	return AsmKernelDispatch(argc, argv).RunMain();
}
 
 
// ------------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------------
Init():
	hsa_agent_get_info(HSA_AGENT_INFO_QUEUE_MAX_SIZE);
	queue = hsa_queue_create();
	signal = hsa_signal_create();
		AqlQueue:Queue,LocalSignal,DoorbellSignal
			Queue:LocalQueue
			LocalSignal:
			DoorbellSignal:Signal
	
InitDispatch():
	hsa_queue_add_write_index_relaxed(queue); 
		write_dispatch_id++;
	aql = queue->base_address + packet_index;
	aql->completion_signal = signal;
	
SetupExecutable():
	SetupCodeObject():
		LoadCodeObjectFromFile():
			hsa_code_object_deserialize(); // include memory copy to device (twice)
	hsa_executable_create();
	
	hsa_executable_load_code_object();
	hsa_executable_freeze(); // segment memory copy
	hsa_executable_get_symbol(); // get "hello_word" kernel_symble
	hsa_executable_symbol_get_info(); // get code_handle
	aql->kernel_object = excu_obj->get code_handle()
	
Setup():
	setup workgroup, kernelparam for aql
	
RunDispatch():
	hsa_signal_store_relaxed();

Wait():
	hsa_signal_wait_acquire();
	
enable_interrupt_ = false
g_use_interrupt_wait = false
Signal = new core::DefaultSignal(0);
queue_full_workaround_ = false
profile_ = HSA_PROFILE_BASE


use_ats = false

CopyTo(): copy memory HostToDevice
	-> CopyToLocal() -> hsa_memory_copy()
CopyFrom(): copy memory DeviceToHost
	-> CopyFromLocal() -> hsa_memory_copy()

AllocateBuffer():
	hsa_memory_allocate()
		core::Runtime::runtime_singleton_->AllocateMemory(); AllocateNoFlags
			region->Allocate()
				AllocateKfdMemory() 	-> hsaKmtAllocMemory()
				MakeKfdMemoryResident() -> hsaKmtMapMemoryToGPUNodes()


HsaMemFlags:
    mem_flag_.ui32.PageSize = HSA_PAGE_SIZE_4KB;
    mem_flag_.ui32.NoSubstitute = 1;
    mem_flag_.ui32.HostAccess = (mem_props_.HeapType == HSA_HEAPTYPE_FRAME_BUFFER_PRIVATE) ? 0 : 1 = 1;
    mem_flag_.ui32.NonPaged = 1;
    map_flag_.ui32.PageSize = HSA_PAGE_SIZE_4KB;
	mem_flag_.ui32.CoarseGrain = (fine_grain) ? 0 : 1; // fine_grain = false; CoarseGrain = 1
	
ioc_flags |= KFD_IOC_ALLOC_MEM_FLAGS_PUBLIC;
ioc_flags |= KFD_IOC_ALLOC_MEM_FLAGS_EXECUTABLE;