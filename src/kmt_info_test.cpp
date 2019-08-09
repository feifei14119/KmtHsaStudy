#include "kmt_test.h"

int kfd_fd;
int gKmtNodeNum;
int gGpuId;

int sys_page_size;
void kmt_info_test()
{
	printf("***********************\n");
	printf("* kmt info test       *\n");
	printf("***********************\n");

	printf("kfd device file = %s.\n", KFD_DEVICE);
	printf("kfd system properties file = %s.\n", KFD_SYSTEM_PROPERTIES);
	printf("kfd node path = %s.\n", KFD_NODES_PATH);
	printf("\n");

	sys_page_size = sysconf(_SC_PAGESIZE);
	printf("page size = %d(Byte).\n", sys_page_size);
	printf("\n");

	struct kfd_ioctl_get_version_args args = { 0 };
	kmtIoctl(kfd_fd, AMDKFD_IOC_GET_VERSION, &args);
	printf("AMDKFD version = %d.%d.\n", args.major_version, args.minor_version);
	printf("\n");

	gKmtNodeNum = 2;
	printf("sysfs node num = folder num under %s = %d.\n", KFD_NODES_PATH, gKmtNodeNum);
	printf("read gpu_id from %s.\n", (KFD_NODES_PATH + string("/x/gpu_id")).data());
	for (int i = 0; i < gKmtNodeNum; i++)
	{
		int gpu_id = readIntKey(KFD_NODES(i) + string("/gpu_id"));
		printf("node %d gpu_id = %d.\n", i, gpu_id);
		if (gpu_id != 0) 
			gGpuId = gpu_id;
	}
	printf("\n");
}
