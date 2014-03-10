/* hoc functions that might be called from mod files should abort if
   actually used.
*/

#include <assert.h>

double* hoc_pgetarg(int iarg) { (void)iarg; assert(0); }
double* getarg(int iarg) { (void)iarg; assert(0); }
char* gargstr(int iarg) { (void)iarg; assert(0); }
int hoc_is_str_arg(int iarg) { (void)iarg; assert(0); }
int ifarg(int iarg) { (void)iarg; assert(0); }
double chkarg(int iarg, double low, double high) {
	(void)iarg; (void)low; (void)high;
	 assert(0);
}
void* vector_arg(int i) { (void) i; assert(0); }
