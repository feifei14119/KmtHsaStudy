#include "kmthsa.h"

int gKfdFd;
int gNodeNum = 2; // node 0 = cpu; node 1 = gpu
int gNodeIdx = 1; // use gpu
int gGpuId;
int gPageSize = 4 * 1024; // system default page size = 4KB

int kmtIoctl(int fd, unsigned long request, void *arg)
{
	int ret;

	do 
	{
		ret = ioctl(fd, request, arg);
	} while (ret == -1 && (errno == EINTR || errno == EAGAIN));

	return ret;
}
uint64_t kmtReadKey(std::string file, std::string key)
{
	int page_size = 4096;
	FILE * fd = fopen(file.data(), "r");

	uint64_t prop_val;
	if (key == "")
	{
		fscanf(fd, "%ul", &prop_val);
		return prop_val;
	}

	char *read_buf = (char*)malloc(page_size);
	int read_size = fread(read_buf, 1, page_size, fd);
	uint32_t prog = 0;
	char * p = read_buf;
	char prop_name[256];

	while (sscanf(p += prog, "%s %llu\n%n", prop_name, &prop_val, &prog) == 2)
	{
		if (strcmp(prop_name, key.data()) == 0)
		{
			return prop_val;
		}
	}

	fclose(fd);
}

void kmtOpen()
{
	printf("kmtOpen\n");

	gKfdFd = open(KFD_DEVICE, O_RDWR | O_CLOEXEC);
	printf("open kfd device: \"%s\". fd = %d.\n", KFD_DEVICE, gKfdFd);
	assert(gKfdFd != 0);
}
void kmtClose()
{
	printf("kmtClose\n");
	close(gKfdFd);
}
void kmtGetInfo()
{
	printf("kmtGetInfo\n");

	printf("get amdkfd version\n");
	struct kfd_ioctl_get_version_args args = { 0 };
	kmtIoctl(gKfdFd, AMDKFD_IOC_GET_VERSION, &args);
	printf("\tamdkfd version = %d.%d.\n", args.major_version, args.minor_version);

	printf("read gpu_id from \"%s\":\n", (KFD_NODES_PATH + string("[node_idx]/gpu_id")).data());
	gGpuId = kmtReadKey(KFD_NODE(gNodeIdx) + string("/gpu_id"));
	printf("\tnode %d gpu_id = %d = 0x%04X.\n", gNodeIdx, gGpuId, gGpuId);
	assert(gGpuId != 0);
}

void KmtHsaInit()
{
	kmtOpen();
	kmtGetInfo();
	kmtInitMem();
	hsaInitSdma();
}
void KmtHsaDeInit()
{
	hsaDeInitSdma();
	kmtDeInitMem();
	kmtClose();
}
