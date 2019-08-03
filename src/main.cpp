#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include "../common/ff_utils.h"

#include "hsa_test.h"

int main(int argc, char *argv[])
{
	hsa_test_open();

	hsa_info_test();
//	hsa_queue_test();
	hsa_mem_test();

	hsa_test_close();
	return 0;
}
