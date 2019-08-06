#include "kmt_test.h"

void kmt_test()
{
	kmt_test_open();

	kmt_info_test();
	kmt_mem_test();

	kmt_test_close();
}

int kmtIoctl(int fd, unsigned long request, void *arg)
{
	int ret;

	do {
		ret = ioctl(fd, request, arg);
	} while (ret == -1 && (errno == EINTR || errno == EAGAIN));

	return ret;
}

int readIntKey(std::string file, std::string key)
{
	int page_size = 4096;
	FILE * fd = fopen(file.data(), "r");

	int prop_val;
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
}
