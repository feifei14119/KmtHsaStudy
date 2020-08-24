#include "kmthsa.h"

static void * events_page = NULL;

// ==================================================================


// ==================================================================


// ==================================================================
HsaEvent * KmtCreateEvent()
{
	HsaEvent *e = (HsaEvent*)malloc(sizeof(HsaEvent));
	memset(e, 0, sizeof(*e));

	struct kfd_ioctl_create_event_args args = { 0 };

	args.node_id = gNodeIdx;
	args.event_type = HSA_EVENTTYPE_SIGNAL;
	args.auto_reset = true;

	if (!events_page)
	{
		events_page = HsaAllocCPU(KFD_SIGNAL_EVENT_LIMIT * 8);
		args.event_page_offset = (uint64_t)KmtGetVmHandle(events_page);
		printf("events_page = 0x%016lX\n", events_page);
	}

	printf("args.node_id = %d\n", args.node_id);
	printf("args.event_type = %d\n", args.event_type);
	printf("args.auto_reset = %d\n", args.auto_reset);
	printf("args.event_page_offset = 0x%016lX\n", args.event_page_offset);
	int rtn = kmtIoctl(gKfdFd, AMDKFD_IOC_CREATE_EVENT, &args);
	printf("return = %d\n", rtn);
	assert(rtn == 0);

	printf("-----------------------------------\n");
	printf("args.event_id = %d\n", args.event_id);
	printf("args.event_slot_index = %d\n", args.event_slot_index);
	printf("args.event_trigger_data = %d\n", args.event_trigger_data);
	printf("args.event_page_offset = 0x%016lX\n", args.event_page_offset);
	return nullptr;

	/*e->EventId = args.event_id;

	if (!events_page && args.event_page_offset > 0)
	{
		events_page = mmap(NULL, KFD_SIGNAL_EVENT_LIMIT * 8, PROT_WRITE | PROT_READ, MAP_SHARED, gKfdFd, args.event_page_offset);
		assert(events_page != MAP_FAILED);
	}
	
	if (args.event_page_offset > 0 && args.event_slot_index < KFD_SIGNAL_EVENT_LIMIT)
		e->EventData.HWData2 = (uint64_t)&events_page[args.event_slot_index];

	e->EventData.EventType = args.event_type;
	e->EventData.HWData1 = args.event_id;
	e->EventData.HWData3 = args.event_trigger_data;
	e->EventData.EventData.SyncVar.SyncVar.UserData = EventDesc->SyncVar.SyncVar.UserData;
	e->EventData.EventData.SyncVar.SyncVarSize = EventDesc->SyncVar.SyncVarSize;*/

	return e;
}
/*void KmtDestroyEvent(HsaEvent *Event)
{
	CHECK_KFD_OPEN();

	if (!Event)
		return HSAKMT_STATUS_INVALID_HANDLE;

	struct kfd_ioctl_destroy_event_args args = { 0 };

	args.event_id = Event->EventId;

	if (kmtIoctl(kfd_fd, AMDKFD_IOC_DESTROY_EVENT, &args) != 0)
		return HSAKMT_STATUS_ERROR;

	free(Event);
	return HSAKMT_STATUS_SUCCESS;
}*/

void RunTestEvent()
{
	HsaEvent * m_pHsaEvent;

	KmtCreateEvent();
}
