#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include "../common/ff_utils.h"

#include "kmt_test.h"
#include "hsa_test.h"

int main(int argc, char *argv[])
{
//	kmt_test();
	hsa_test();
	return 0;
}
