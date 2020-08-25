#include "kmthsa.h"

static void * gEventsPage = NULL;

// ==================================================================

HsaEvent * KmtCreateEvent()
{
	printf("KmtCreateEvent\n");
	
	printf("malloc HsaEvent\n");
	HsaEvent *evt = (HsaEvent*)malloc(sizeof(HsaEvent));
	memset(evt, 0, sizeof(*evt));

	struct kfd_ioctl_create_event_args args = { 0 };

	args.node_id = gNodeIdx;
	args.event_type = HSA_EVENTTYPE_SIGNAL;
	args.auto_reset = true;

	if (!gEventsPage)
	{
		printf("malloc EventsPage\n");
		gEventsPage = KmtAllocHost(KFD_SIGNAL_EVENT_LIMIT * 8, false);
		KmtMapToGpu(gEventsPage, KFD_SIGNAL_EVENT_LIMIT * 8);
		args.event_page_offset = (uint64_t)KmtGetVmHandle(gEventsPage);
	}

	//printf("args.node_id = %d\n", args.node_id);
	//printf("args.event_type = %d\n", args.event_type);
	//printf("args.auto_reset = %d\n", args.auto_reset);
	//printf("args.event_page_offset = 0x%016lX\n", args.event_page_offset);
	printf("ioctrl create event\n");
	int rtn = kmtIoctl(gKfdFd, AMDKFD_IOC_CREATE_EVENT, &args);
	assert(rtn == 0);
	//printf("-----------------------------------\n");
	//printf("args.event_id = %d\n", args.event_id);
	//printf("args.event_slot_index = %d\n", args.event_slot_index);
	//printf("args.event_trigger_data = %d\n", args.event_trigger_data);
	//printf("args.event_page_offset = 0x%016lX\n", args.event_page_offset);

	printf("fill event object\n");
	evt->EventId = args.event_id;
	evt->EventData.EventType = (HSA_EVENTTYPE)args.event_type;
	evt->EventData.HWData1 = args.event_id;
	evt->EventData.HWData2 = (uint64_t)&((HsaEvent*)gEventsPage)[args.event_slot_index];
	evt->EventData.HWData3 = args.event_trigger_data;
	evt->EventData.EventData.SyncVar.SyncVar.UserData = nullptr;
	evt->EventData.EventData.SyncVar.SyncVarSize = 0;

	printf("event.event_id = %d\n", evt->EventId);
	printf("EventData.EventType = %d\n", evt->EventData.EventType);
	printf("EventData.HWData1 = 0x%016lX\n", evt->EventData.HWData1);
	printf("EventData.HWData2 = 0x%016lX\n", evt->EventData.HWData2);
	printf("EventData.HWData3 = 0x%016lX\n", evt->EventData.HWData3);
	printf("SyncVar.UserData = 0x%016lX\n", evt->EventData.EventData.SyncVar.SyncVar.UserData);
	printf("SyncVar.SyncVarSize = %ld\n", evt->EventData.EventData.SyncVar.SyncVarSize);

	return evt;
}
void KmtDestroyEvent(HsaEvent *Event)
{
	struct kfd_ioctl_destroy_event_args args = { 0 };
	args.event_id = Event->EventId;

	int rtn = kmtIoctl(gKfdFd, AMDKFD_IOC_DESTROY_EVENT, &args);
	assert(rtn == 0);

	free(Event);
}
void KmtSetEvent(HsaEvent *Event)
{
	struct kfd_ioctl_set_event_args args = { 0 };
	args.event_id = Event->EventId;

	int rtn = kmtIoctl(gKfdFd, AMDKFD_IOC_SET_EVENT, &args);
	assert(rtn == 0);
}
void KmtWaitOnEvent(HsaEvent *Event)
{
	printf("KmtWaitOnEvent\n");

	printf("malloc kfd_event_data\n");
	struct kfd_event_data *event_data = (kfd_event_data *)calloc(1, sizeof(struct kfd_event_data));
	event_data->event_id = Event->EventId;
	event_data->kfd_event_data_ext = (uint64_t)(uintptr_t)NULL;

	struct kfd_ioctl_wait_events_args args = { 0 };
	args.wait_for_all = true;
	args.timeout = 1000;
	args.num_events = 1;
	args.events_ptr = (uint64_t)(uintptr_t)event_data;

	printf("ioctrl wait event\n");
	int rtn = kmtIoctl(gKfdFd, AMDKFD_IOC_WAIT_EVENTS, &args);
	assert(rtn != -1);

	if (args.wait_result == KFD_IOC_WAIT_RESULT_TIMEOUT)
	{
		printf("HSAKMT_STATUS_WAIT_TIMEOUT\n");
	}
	else
	{
		printf("HSAKMT_STATUS_SUCCESS\n");
	}
}

// ==================================================================

void RunEventTest()
{
	HsaEvent * evt;

	evt = KmtCreateEvent();
	KmtWaitOnEvent(evt);

	KmtSetEvent(evt);
	KmtWaitOnEvent(evt);
	
	KmtDestroyEvent(evt);
}
