#pragma once

#include "ff_basic.h"

namespace feifei
{
#define	PI					(3.1415926535897932384626)
#define	TWO_PI				(6.28318530717958647692528676655)
#define	PI_SP				(3.1415926535897932384626F)
#define	TWO_PI_SP			(6.28318530717958647692528676655F)
#define FLT_MAX				(3.402823466e+38f)
#define	MIN_FP32_ERR		(1e-6)

#define MAX_16BIT_UINT		(65535)

#define _next2pow(n)	do{ int base = 1; \
	for (int i = 0; i < 32; i++) { base = 1 << i; if (n <= base) { break; }} \
	return base; } while (0)

#define _log2(value)	do{ int log2 = 0; \
	while (value > 1) { value = value / 2; log2++; } \
	return log2; } while(0)

#define _isPow2(value)  do{ return ((value & (value - 1)) == 0);} while(0)

#define _divCeil(a,b)	((a + b - 1) / b)

}