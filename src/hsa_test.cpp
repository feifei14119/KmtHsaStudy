#include <string>
#include <iostream>
#include <vector>

#include "hsa_test.h"

using namespace std;

void hsa_test()
{
	hsa_test_open();

	hsa_info_test();
	hsa_mem_test();

	hsa_test_close();
}
