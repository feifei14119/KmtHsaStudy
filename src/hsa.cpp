#include <string>
#include <iostream>
#include <vector>

#include "kmt_test.h"

using namespace std;

void HsaInit()
{
	KmtInit();
	HsaInitSdma();
}
void HsaDeInit()
{
	HsaDeInitSdma();
	KmtDeInit();
}

