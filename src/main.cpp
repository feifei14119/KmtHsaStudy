#include "kmthsa.h"

const string CodeBinFile = "../kernel/VectorAdd.co";

void RunSubTest()
{
	printf("++++++++++++++++ RunSubTest ++++++++++++++++++\n");
	RunMemoryTest();
	//RunKmtTest();
	//RunEventTest();
	//RunSdmaTest();

	printf("++++++++++++++ KmtHsaDeInit ++++++++++++++++++\n");
	KmtHsaDeInit();
}

int main(int argc, char *argv[])
{
	printf("++++++++++++++ KmtHsaInit ++++++++++++++++++++\n");
	KmtHsaInit();

	//RunSubTest(); return 0;

	printf("++++++++++++++ Prepare Data ++++++++++++++++++\n");
	uint32_t len = 1024;
	float * h_A, *h_B, *h_C;
	float * d_A, *d_B, *d_C;

	h_A = (float*)HsaAllocCPU(len * sizeof(float));
	h_B = (float*)HsaAllocCPU(len * sizeof(float));
	h_C = (float*)HsaAllocCPU(len * sizeof(float));
	d_A = (float*)HsaAllocGPU(len * sizeof(float));
	d_B = (float*)HsaAllocGPU(len * sizeof(float));
	d_C = (float*)HsaAllocGPU(len * sizeof(float));

	for (uint32_t i = 0; i < len; i++)
		h_A[i] = i * 1.0f;
	for (uint32_t i = 0; i < len; i++)
		h_B[i] = i*0.1f;

	printf("++++++++++++++++ Copy To Gpu +++++++++++++++++\n");
	HsaSdmaCopy(d_A, h_A, len * sizeof(float));
	HsaSdmaCopy(d_B, h_B, len * sizeof(float));

	printf("++++++++++++++++ Load Kernel +++++++++++++++++\n");
	void * KernelHandle = HsaLoadKernel(CodeBinFile);

	printf("++++++++++++++++ Create Aql ++++++++++++++++++\n");
	HsaAqlCreate();
	printf("++++++++++++++++ Launch Aql ++++++++++++++++++\n");
	HsaAqlSetPkt(KernelHandle, 256, 4*256);
	HsaAqlSetKernelArg((void**)&d_A, sizeof(d_A));
	HsaAqlSetKernelArg((void**)&d_B, sizeof(d_B));
	HsaAqlSetKernelArg((void**)&d_C, sizeof(d_C));
	HsaAqlRingDoorbell();
	usleep(1000);

	printf("++++++++++++++++ Copy Back +++++++++++++++++++\n");
	HsaSdmaCopy(h_C, d_C, len * sizeof(float));

	printf("++++++++++++++++ Result ++++++++++++++++++++++\n");
	for (uint32_t i = 0; i < 16; i++)
	{
		printf("%.1f, ", h_C[i]);
		if ((i + 1) % 8 == 0)
			printf("\n");
	}
	printf("... ...\n");
	for (uint32_t i = len - 16; i < len; i++)
	{
		printf("%.1f, ", h_C[i]);
		if ((i + 1) % 8 == 0)
			printf("\n");
	}
	printf("\n");

	printf("++++++++++++++ KmtHsaDeInit ++++++++++++++++++\n");
	HsaFreeMem(h_A);
	HsaFreeMem(h_B);
	HsaFreeMem(h_C);
	HsaFreeMem(d_A);
	HsaFreeMem(d_B);
	HsaFreeMem(d_C);

	KmtHsaDeInit();
	return 0;
}

