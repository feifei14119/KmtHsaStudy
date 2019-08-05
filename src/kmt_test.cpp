#include <string>
#include <iostream>
#include <vector>

#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <strings.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>

#include "kmt_test.h"

using namespace std;

static const char kfd_device_name[] = "/dev/kfd";
int kfd_fd;

/* Call ioctl, restarting if it is interrupted */
int kmtIoctl(int fd, unsigned long request, void *arg)
{
	int ret;

	do {
		ret = ioctl(fd, request, arg);
	} while (ret == -1 && (errno == EINTR || errno == EAGAIN));

	return ret;
}


void kmt_test()
{
	kmt_test_open();


	struct kfd_ioctl_get_version_args args = { 0 };
	kmtIoctl(kfd_fd, AMDKFD_IOC_GET_VERSION, &args);
	printf("AMDKFD version = %d.%d.\n", args.major_version, args.minor_version);


	kmt_test_close();
}
