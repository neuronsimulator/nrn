#include <../../nrnconf.h>
#ifdef WIN32
#include <windows.h>
#endif
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <OS/string.h>
#include <OS/file.h>
#include <InterViews/regexp.h>
#include "nrnbbs.h"

#ifdef WIN32
#include "../winio/debug.h"
static void setneuronhome(const char*);
#endif

extern const char* neuronhome();

void start();
void stop();
void send(const char* url);

static bool quit_;

static void quit(const char* msg) {
//printf("hel2mos quit |%s|\n", msg);
//DebugMessage("hel2mos quit |%s|\n", msg);
	if (strcmp(msg, "neuron closed") == 0) {
		quit_ = true;
	}
}

static RETSIGTYPE quit1(int) {
	quit_ = true;
}

static void help(const char*);
static CopyString* shelp;


int main(int argc, const char** argv){
#ifdef WIN32
	setneuronhome(argv[0]);
#endif
	if (!neuronhome()) {
		printf("no NEURONHOME\n");
		return 1;
	}
//	printf("started hel2mos\n");

	char buf[256];
	sprintf(buf, "%s/lib/helpdict", neuronhome());
	String sf(buf);
	InputFile* f = InputFile::open(sf);
	if (f == nil) {
		printf("Can't open %s\n", sf.string());
		return 1;
	}
	const char* st;
	int flen = f->read(st);
	shelp = new CopyString(st, flen);
	f->close();

	nrnbbs_connect();
	nrnbbs_post("ochelp running");
	start();
	nrnbbs_notify("ochelp", help);
#ifdef WIN32
	nrnbbs_notify("neuron closed", help);
#else
	nrnbbs_notify("neuron closed", quit);
#endif
	help("");
	nrnbbs_wait(&quit_);
	stop();
	nrnbbs_take("ochelp running");
	nrnbbs_disconnect();
	return 0;
}

static bool find(const char* key, char* val) {
	static char buf[256];
	sprintf(buf, "^%s ", key);
//printf("|%s|\n", buf);
	Regexp r1(buf);
	int i = r1.Search(shelp->string(), shelp->length(), 0, shelp->length());
//printf("i=%d\n", i);
	if (i < 0) {
		return false;
	}
	Regexp r2("neuron/");
	i = r2.Search(shelp->string(), shelp->length(), i, shelp->length());
//printf("i=%d\n", i);
	int j = shelp->search(i, '\n');
//printf("j=%d\n", j);
	strncpy(val, shelp->string() + i, j-i);
	val[j-i] = '\0';
	return true;
}

static void help(const char* msg) {
	if (strcmp(msg, "neuron closed") == 0) {
		quit_ = true;
	}
	char buf[256];
//printf("hel2mos help |%s|\n", msg);
	while(nrnbbs_take_string("ochelp", buf)) {
		if (buf[0] == '?') {
//			printf("took ochelp: %s\n", buf);
		}else{
			if (find(buf, buf)) {
//				printf("%s\n", buf);
			}else{
				sprintf(buf, "contents.html");
			}
#ifdef WIN32
//         DebugMessage("buf=|%s|\n", buf);
		char buf1[256],buf2[256];
		strcpy(buf2,buf);
		strcpy(buf1, neuronhome());
		for (char* cp = buf1; *cp; ++cp) {
			if (*cp == ':') {
				*cp = '|';
			}
			if (*cp == '\\') {
				*cp = '/';
			}
		}
		sprintf(buf, "file:///%s/html/help/%s", buf1, buf2);
#endif
			send(buf);
		}		
	}
}

const char* neuronhome() {
	const char* n = getenv("NEURONHOME");
	if (n) {
		return n;
	}
	return nil;
}

#if defined(WIN32)
static void setneuronhome(const char* p) {
	// if the program lives in .../bin/mos2nrn.exe
	// and .../lib exists then use ... as the
	// NEURONHOME
//   printf("p=|%s|\n", p);
	char buf[256];
   if (p[0] == '"') {
   	strcpy(buf, p+1);
   }else{
		strcpy(buf, p);
   }
   int i, j;
   for (i=strlen(buf); i >= 0 && buf[i] != '\\'; --i) {;}
   buf[i] = '\0'; // /neuron.exe gone
	//printf("setneuronhome |%s|\n", buf);
   for (j=strlen(buf); j >= 0 && buf[j] != '\\'; --j) {;}
   buf[j] = '\0'; // /bin gone
   // but make sure it was bin Bin or BIN -- damn you bill gates
	//printf("i=%d j=%d buf=|%s|\n",i, j, buf);
   if (i == j+4
    	&&(buf[--i] == 'n' || buf[i] == 'N')
    	&&(buf[--i] == 'i' || buf[i] == 'I')
   	&&(buf[--i] == 'b' || buf[i] == 'B')
      ) {
			static char buf1[256];
			// check for nrn.def or nrn.defaults
			// if it exists assume valid installation
			FILE* f;
			sprintf(buf1, "%s/lib/nrn.def", buf);
			if ((f = fopen(buf1, "r")) == (FILE*)0) {
				sprintf(buf1, "%s/lib/nrn.defaults", buf);
				if ((f = fopen(buf1, "r")) == (FILE*)0) {
					sprintf(buf1, "%s not valid neuronhome\n", buf);
					MessageBox(NULL, buf1, "mos2nrn", MB_OK);
					return;
				}
			}
			fclose(f);
			sprintf(buf1, "NEURONHOME=%s", buf);
			putenv(buf1); // arg must be global
	}
}

char* nrnhome;

#else

#include <unistd.h>
#include <signal.h>

static int mosaic_pid_; /* no longer used. 0 is fine */

void start() {
#if defined(SIGNAL_CAST)
	signal(SIGHUP, (SIGNAL_CAST)quit1);
#else
	signal(SIGHUP, quit1);
#endif
}

void stop() {
}

void send(const char* url) {
	char buf1[512];
	int start = 0;
	while(url[start] == ' ') {
		++start;
	}
	sprintf(buf1, "%s/bin/hel2mos1.sh \"%s\"", neuronhome(), url+start);
//printf("sending |%s|\n", buf1);
	signal(SIGCHLD, SIG_IGN);
	system(buf1);
#if defined(SIGNAL_CAST)
	signal(SIGCHLD, (SIGNAL_CAST)quit1);
#else
	signal(SIGCHLD, quit1);
#endif
}

#endif
