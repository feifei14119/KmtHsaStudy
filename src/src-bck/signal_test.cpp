#include "kmt_test.h"

static uint64_t * events_page = NULL;

// ==================================================================
// ==================================================================
extern void * find_vm_handle(void * memAddr);
HsaEvent * CreateSignal()
{
	/*
	 * 每个 event 为 8Byte
	 * 最多 4096 个 event
	 */
	if (events_page == NULL)
		events_page = (uint64_t*)HsaAllocCPU(KFD_SIGNAL_EVENT_LIMIT * 8);

	struct kfd_ioctl_create_event_args args = { 0 };

	args.node_id = 0;
	args.auto_reset = true;
	args.event_type = HSA_EVENTTYPE_SIGNAL;
	args.event_page_offset = (uint64_t)find_vm_handle(events_page);

	int rtn = kmtIoctl(gKfdFd, AMDKFD_IOC_CREATE_EVENT, &args);
	assert(rtn == 0);

	if (!events_page && args.event_page_offset > 0)
	{
		events_page = (uint64_t *)mmap(NULL, KFD_SIGNAL_EVENT_LIMIT * 8, PROT_WRITE | PROT_READ, MAP_SHARED, gKfdFd, args.event_page_offset);
		assert(events_page == MAP_FAILED);
	}
		
	HsaEvent * evt = (HsaEvent*)malloc(sizeof(HsaEvent));
	memset(evt, 0, sizeof(HsaEvent));
	evt->EventId = args.event_id;
	evt->EventData.EventType = HSA_EVENTTYPE_SIGNAL;
	evt->EventData.HWData1 = args.event_id;
	evt->EventData.HWData2 = (uint64_t)&events_page[args.event_slot_index];
	evt->EventData.HWData3 = args.event_trigger_data;
	evt->EventData.EventData.SyncVar.SyncVar.UserData = NULL;// EventDesc->SyncVar.SyncVar.UserData;
	evt->EventData.EventData.SyncVar.SyncVarSize = sizeof(size_t);// EventDesc->SyncVar.SyncVarSize;

	printf("\n=======================\n");
	printf("create event.\n");
	printf("\tevent id = %d.\n", args.event_id);
	printf("\tevent slot idx  = %d.\n", args.event_slot_index);
	printf("\tevent trig data = %d.\n", args.event_trigger_data);
	printf("\tevent page offset = 0x%016lX.\n", args.event_page_offset);
	printf("\tevent page offset = 0x%016lX.\n", events_page);

	return evt;
}
void SetEvent(HsaEvent * evt)
{
	struct kfd_ioctl_set_event_args args = { 0 };
	args.event_id = evt->EventId;
	int rtn = kmtIoctl(gKfdFd, AMDKFD_IOC_SET_EVENT, &args);
	assert(rtn == 0);
}

bool WaitEvent(HsaEvent * evt)
{
	struct kfd_event_data event_data;

		event_data.event_id = evt->EventId;
		event_data.kfd_event_data_ext = (uint64_t)(uintptr_t)NULL;

	struct kfd_ioctl_wait_events_args args = { 0 };

	args.wait_for_all = true;
	args.timeout = 1000;// Milliseconds;
	args.num_events = 1;
	args.events_ptr = (uint64_t)(uintptr_t)&event_data;
	
	int rtn = kmtIoctl(gKfdFd, AMDKFD_IOC_WAIT_EVENTS, &args);
	if (rtn == -1)
	{
		printf("wait event failed.\n");
		return false;
	}
	else if (args.wait_result == KFD_IOC_WAIT_RESULT_TIMEOUT)
	{
		printf("wait event time out.\n");
		return false;
	}
	else
	{
		printf("wait event success.\n");
		return true;
		/*if (Events->EventData.EventType == HSA_EVENTTYPE_MEMORY && event_data.memory_exception_data.gpu_id)
		{
			Events->EventData.EventData.MemoryAccessFault.VirtualAddress = event_data.memory_exception_data.va;
			Events->EventData.EventData.MemoryAccessFault.Failure.NotPresent = event_data.memory_exception_data.failure.NotPresent;
			Events->EventData.EventData.MemoryAccessFault.Failure.ReadOnly = event_data.memory_exception_data.failure.ReadOnly;
			Events->EventData.EventData.MemoryAccessFault.Failure.NoExecute = event_data.memory_exception_data.failure.NoExecute;
			Events->EventData.EventData.MemoryAccessFault.Failure.Imprecise = event_data.memory_exception_data.failure.imprecise;
			Events->EventData.EventData.MemoryAccessFault.Failure.ErrorType = event_data.memory_exception_data.ErrorType;
			Events->EventData.EventData.MemoryAccessFault.Failure.ECC = ((event_data.memory_exception_data.ErrorType == 1) || (event_data.memory_exception_data.ErrorType == 2)) ? 1 : 0;
			Events->EventData.EventData.MemoryAccessFault.Flags = HSA_EVENTID_MEMORY_FATAL_PROCESS;
			analysis_memory_exception(&event_data.memory_exception_data);
		}*/
	}

	return true;
}

void SignalTest()
{
	KmtInitMem();
	printf("***********************\n");
	printf("* signal test      *\n");
	printf("***********************\n");
	
	HsaEvent * evt = CreateSignal();
	SetEvent(evt);
	WaitEvent(evt);

	printf("\n");
	KmtDeInitMem();
}

// event_type = HSA_EVENTTYPE_SIGNAL
// ManualReset = false
// IsSignaled = false --> not signal

