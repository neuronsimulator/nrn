#define _ISOC99_SOURCE
#include <math.h>
#include <stdlib.h>
/* trying to safely determine if a pointer is a pointer to a double where the
double value would be in a specific range without causing floating exceptions
Of course this founders in general on the dereferencing of invalid memory
so it can only be used when pd is valid over sizeof(double).
*/
int nrn_isdouble(void* pd, double min, double max) {
	int i;
	if (!pd) { return 0; }
#if defined(fpclassify)
	i = fpclassify(*((double*)pd));
	if (i == FP_NORMAL || i == FP_ZERO) {
		return *((double*)pd) >= min && *((double*)pd) <= max;
	}else{
		return 0;	
	}
#else
	return *((double*)pd) >= min && *((double*)pd) <= max;
#endif
}
