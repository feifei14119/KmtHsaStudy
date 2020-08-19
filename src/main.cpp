#include "hsa.h"

int main(int argc, char *argv[])
{
	HsaInit();
	
	uint32_t len = 1024;
	float * h_A, *h_B, *d_C;

	h_A = (float*)HsaAllocCPU(len * sizeof(float));
	h_B = (float*)HsaAllocCPU(len * sizeof(float));
	d_C = (float*)HsaAllocGPU(len * sizeof(float));

	for (uint32_t i = 0; i < len; i++)
		h_A[i] = i * 1.0f;
	for (uint32_t i = 0; i < len; i++)
		h_B[i] = 0;

	HsaSdmaCopy(d_C, h_A, len * sizeof(float));
	HsaSdmaCopy(h_B, d_C, len * sizeof(float));

	printf("=============================\n");
	printf("source host data:\n");
	for (uint32_t i = 0; i < 16; i++)
	{
		printf("%.1f, ", h_A[i]);
		if ((i + 1) % 8 == 0)
			printf("\n");
	}
	printf("... ...\n");
	for (uint32_t i = len - 16; i < len; i++)
	{
		printf("%.1f, ", h_A[i]);
		if ((i + 1) % 8 == 0)
			printf("\n");
	}
	printf("\ndest host data:\n");
	for (uint32_t i = 0; i < 16; i++)
	{
		printf("%.1f, ", h_B[i]);
		if ((i + 1) % 8 == 0)
			printf("\n");
	}
	printf("... ...\n");
	for (uint32_t i = len - 16; i < len; i++)
	{
		printf("%.1f, ", h_B[i]);
		if ((i + 1) % 8 == 0)
			printf("\n");
	}
	printf("=============================\n");

	HsaFreeMem(h_A);
	HsaFreeMem(h_B);
	HsaFreeMem(d_C);

	printf("\n");
	return 0;
}
