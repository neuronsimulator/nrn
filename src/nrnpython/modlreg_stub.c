/* libnrniv leaves modl_reg undefined for compiled mechanism libraries to
 * provide; the legacy hoc extension module supplied an empty one. The Python
 * interface loads this library (RTLD_GLOBAL) before libnrniv instead. */
void modl_reg(void) {}
