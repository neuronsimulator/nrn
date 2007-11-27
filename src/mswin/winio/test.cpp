#include <windows.h>
#include <winio.h>
extern "C" { int fgetchar(void);}

int main(int argc, char** argv) {
	int c;
	int i;
	printf("hello\n");
	printf("argc = %d\n", argc);
	for (i = 0; i < argc; ++i) {
	 printf("argv[%d] = %s\n", i, argv[i]);
	}
	while((c = fgetchar()) != -1) {
		printf("%o", c);
	}

	return 0;
}
extern "C" {
void single_event_run(){}
void set_intset(){}
void hoc_quit() {
	winio_closeall();
	return;
}

}
