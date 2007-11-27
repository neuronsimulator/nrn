#include <../../nrnconf.h>
#include <stdio.h>
#include "nrnbbs.h"

void cb(const char* s) {
	printf("callback %s\n", s);
	char buf[200];
   buf[0] = '\0';
	int ok = nrnbbs_take_string(s, buf);
	printf("%d take |%s| |%s|\n", ok, s, buf);
}

void main() {
	int i, ok;
	char buf[256];
	nrnbbs_connect();
	nrnbbs_notify("test2", cb);
	nrnbbs_post("test1");
	nrnbbs_post_int("test2", 5);
	nrnbbs_post_string("test3", "posted string");

	printf ("test1 look %d\n", nrnbbs_look("test1"));
	printf ("test1 look %d\n", nrnbbs_look("test1"));
	printf ("test1 take %d\n", nrnbbs_take("test1"));
	printf ("test1 take %d\n", nrnbbs_take("test1"));

	i=0;
	ok = nrnbbs_take_int("test2", &i);
	printf("test2 %d i=%d\n", ok, i);

	nrnbbs_post("space look");
	printf( "space look %d\n", nrnbbs_look("space look"));
   printf( "space take %d\n", nrnbbs_take("space look"));
	buf[0] = '\0';
	ok = nrnbbs_take_string("test3", buf);
   printf("test3 %d buf=|%s|\n", ok, buf);

	printf("Hit Return:\n");
	gets(buf);
	nrnbbs_disconnect();
}
