#include <stdio.h>
#include <stdlib.h>
int main(int argc, char** argv) {
	char *cp1, *cp2;
	char buf[256];
	int i;
	for (i=1; i < argc; ++i) {
		if (i > 1) {
			printf(" ");
		}
		for (cp1=argv[i], cp2=buf; *cp1; ++cp1, ++cp2) {
			if (*cp1 == '\\') {
				*cp2 = '/';
			}else{
				*cp2 = *cp1;
			}
		}
      *cp2 = '\0';
		printf("%s", buf);
	}
	printf("\n", buf);
	return 0;
}
