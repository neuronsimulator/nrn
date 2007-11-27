#include <stdio.h>
void winio_key_press() {
//	printf("winio_key_press\n");
}

int iv_windows_exist() {
	return 0;
}

#if !HOC
void hoc_quit() {
	winio_closeall();
	return;
}

void set_intset() {}

#endif
