/* split a file.zip file into 1MB pieces called file.z1, file.z2 etc */

#include <stdio.h>
#include <malloc.h>
#include <stdlib.h>

void main(int argc, char** argv) {
	long i, j, n, done;
	FILE* fi, *fo;
	char* buf;
	buf = (char*)malloc(1024*16);
	if (argc != 2) {
		printf("usage: splitnrn nrndisk2\n");
		exit(1);
	}
	sprintf(buf, "%s.zip", argv[1]);
	if ((fi = fopen(buf, "rb")) == (FILE*)0) {
		printf("can't open %s\n", buf);
		exit(1);
	}
	n=1;
   done = 0;
	while (!done) {
		sprintf(buf, "%s.z%d", argv[1], n);
		if ((fo = fopen(buf, "wb")) == (FILE*)0) {
			printf("can't open %s\n", buf);
			exit(1);
		}
		for (j=0; j < 88; ++j) { /* 88*1024*16 = 1441792 */
			i = fread(buf, sizeof(char), 1024*16, fi);
			printf("read %d\n", i);
			i = fwrite(buf, sizeof(char), i, fo);
         printf("write %d\n", i);
			if (i < 1024*16) {
				done = 1;
				break;
			}
		}
		++n;
		fclose(fo);
	}
}

