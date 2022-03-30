// prints the first "x:.../activate.bat" in the first 10000 bytes of
// the binary file (second arg). Characters may or may not be 16 bit
// integers (ignore '\0')
// (return 0 if found, 1 if not found or error)

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

int main(int argc, const char** argv) {
    if (argc != 3) {
        fprintf(stderr, "Usage: nrnbinstr \"pattern\" \"binaryfile\"\n");
        return 1;
    }
    const char* pat = argv[1];
    const char* fname = argv[2];

    FILE* f = fopen(fname, "rb");
    if (!f) {
        fprintf(stderr, "Could not open %s\n", fname);
        return 1;
    }

    char* buf = new char[10000];
    int size = fread(buf, sizeof(char), 10000, f);
    fclose(f);

    int c;
    char* result = new char[10000];
    int lenpat = strlen(pat);
    int ifirst = 0, ipat = 0, iresult = 0;
    int icolon = 0;
    for (int i = 0; i < size - 1; ++i) {
        c = buf[i];
        if (c == 0 && buf[i + 1] != 0) {
            continue;
        }  // ignore single '\0'
        if (isprint(c)) {
            if (c == ':' && iresult > 0) {
                icolon = iresult - 1;  // must be single character drive letter
            }
            result[iresult++] = c;
            if (c == pat[ipat]) {
                ipat++;
                if (ipat == lenpat) {
                    result[iresult] = '\0';
                    printf("%s\n", result + icolon);
                    return 0;
                }
            } else if (ipat > 0) {
                ipat = 0;
            }
        } else {
            icolon = 0;
            iresult = 0;
            ipat = 0;
            ifirst = i + 1;
        }
    }
    return 1;
}
