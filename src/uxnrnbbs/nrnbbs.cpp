#include <../../nrnconf.h>
// bug in the osx 10.2 environment
#if defined(__POWERPC__) && defined(__APPLE__)
#undef HAVE_LOCKF
#endif

#include <stdio.h>
#include <fcntl.h>
#include <ctype.h>
#include <signal.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include <sys/stat.h>
//extern "C" { extern int mkdir(const char*, int); }

#include <OS/list.h>
#include <OS/string.h>
#include "nrnbbs.h"

#if !defined(HAVE_LOCKF)
#include <sys/file.h>
#endif

#define HISTORY 0

#define NRNBBSTMP "nrnbbs"
#define NOTIFY "notify"
#define NRNBBS "nrnbbs"
#define LOCKFILE "lock"
#define TMPFILE "temp"
#define NOTIFY_SIGNAL SIGUSR1

class NrnBBSCallbackItem {
public:
	NrnBBSCallbackItem(const char*, NrnBBSCallback);
	virtual ~NrnBBSCallbackItem();
	bool equal(const char*);
	void execute();
private:
	CopyString s_;
	NrnBBSCallback cb_;
};

static const char* fname(const char* name);

static void history(const char* s1, const char* s2, const char* s3) {
#if HISTORY
	FILE*f;
	if ((f = fopen(fname("history"), "a")) != (FILE*)0) {
		fprintf(f, "%5d %s  |%s| |%s|\n", getpid(), s1, s2, s3);
		fclose(f);
	}
#endif
}

static void history(const char* s1) { history(s1, "", ""); }
static void history(const char* s1, const char* s2) { history (s1, s2, ""); }
static void history(const char* s1, int i) {
	char buf[20];
	sprintf(buf, "%d", i);
	history(s1, buf, "");
}

declarePtrList(NrnBBSCallbackList, NrnBBSCallbackItem)
implementPtrList(NrnBBSCallbackList, NrnBBSCallbackItem)

static NrnBBSCallbackList* cblist_;
static FILE* lockfile_;

static void get_lock() {
#if !defined(HAVE_LOCKF)
	while (flock(fileno(lockfile_), LOCK_EX) != 0) {
#else
	while (lockf(fileno(lockfile_), F_LOCK, 0) != 0) {
#endif
		history("lockf returned non zero");
	}
//printf("got lock for %d\n", getpid());
}

static void release_lock() {
#if !defined(HAVE_LOCKF)
	flock(fileno(lockfile_), LOCK_UN);
#else
	lockf(fileno(lockfile_), F_ULOCK, 0);
#endif
//printf("released lock for %d\n", getpid());
}

static const char* nrnbbsdir() {
	static char* d ;
	if (!d) {
		char buf[256];
		char* tmpdir;
		tmpdir = getenv("TMPDIR");
		if (!tmpdir) {
			tmpdir = "/tmp";
		}
		sprintf(buf, "%s/%s_%s", tmpdir, getenv("USER"), NRNBBSTMP);
		d = new char[strlen(buf)+1];
		strcpy(d, buf);
		mkdir(d, 0777);
	}
	return d;
}

static const char* fname(const char* name) {
	static char buf[2][256];
	static int i = 0;
	i = (i+1)%2;
	sprintf(buf[i], "%s/%s", nrnbbsdir(), name);
	return buf[i];
}

static bool connected_;

bool nrnbbs_connect() {
	if (!lockfile_) {
		const char* lfile = fname(LOCKFILE);
		lockfile_ = fopen(lfile, "w");
		if (!lockfile_) {
			connected_ = false;
			return connected_;
		}
	}
	connected_ = true;
	history("connect");
	return connected_;
}

void nrnbbs_disconnect() {
	connected_ = false;
	history("disconnect");
	if (cblist_) {
		get_lock();
		int pid = getpid();
		FILE* f = fopen(fname(NOTIFY), "r");
		if (f) {
			char buf[256];
			int id;
			FILE* f2 = fopen(fname(TMPFILE), "w");
			if (!f2) {
				fprintf(stderr, "can't open %s\n", fname(TMPFILE));
			}
			while (fgets(buf, 256, f)) {
				buf[strlen(buf) - 1] = '\0';
				fscanf(f, "%d\n", &id);
				if (id != pid) {
					fprintf(f2, "%s\n%d\n", buf, id);
				}
			}
			fclose(f2);
			fclose(f);
			rename(fname(TMPFILE), fname(NOTIFY));
		}
		release_lock();
		for (long i=0; i < cblist_->count(); ++i) {
			delete cblist_->item(i);
		}
		delete cblist_;
		cblist_ = nil;
	}
	if (lockfile_) {
		fclose(lockfile_);
		lockfile_ = (FILE*)0;
	}
}

bool nrnbbs_connected() {
	return connected_;
}


void nrnbbs_wait(bool* pflag) {
	bool f = false;
	bool *pf;	
	pf = (pflag) ? pflag : &f;
	while (!(*pf) && nrnbbs_connected()) {
		f = true; // once only if no arg
		pause();
	}
}

void nrnbbs_post(const char* key) {
	nrnbbs_post_string(key, "");
}

void nrnbbs_post_int(const char* key, int ival) {
	char buf[30];
	sprintf(buf, "%d", ival);
	nrnbbs_post_string(key, buf);
}

void nrnbbs_post_string(const char* key, const char* sval) {
	history("post", key, sval);
	get_lock();
	FILE* f = fopen(fname(NRNBBS), "a");
	fprintf(f, "%s\n%s\n", key, sval);
	FILE* f2 = fopen(fname(NOTIFY), "r");
	char name[256];
	int i, n, id[10];
	n = 0;
	if (f2) {
		while( fgets(name, 256, f2) && n < 10) {
			name[strlen(name) - 1] = '\0';
			fscanf(f2, "%d\n", &i);
			if (strcmp(name, key) == 0) {
				id[n++] = i;
				fprintf(f, "nrnbbs_notifying %s\n\n", key);
			}
		}
		fclose(f2);
	}
	fclose(f);
	release_lock();
	for (i=0; i < n; ++i) {
		history("  notify", id[i]);
		kill(id[i], NOTIFY_SIGNAL);
	}
}


bool nrnbbs_take(const char* key) {
	char buf[256];
	return nrnbbs_take_string(key, buf);
}

bool nrnbbs_take_int(const char* key, int* ipval) {
	char buf[256];
	bool b = nrnbbs_take_string(key, buf);
	if (b) {
		b = (sscanf(buf, "%d\n", ipval) == 1) ? true : false;
	}
	return b;
}

bool nrnbbs_take_string(const char* key, char* sval) {
	history("take", key);
	get_lock();
	bool b = false;
	FILE* f = fopen(fname(NRNBBS), "r");
	if (f != (FILE*)0) {
		char name[256], val[256];
		FILE* f2 = fopen(fname(TMPFILE), "w");
		while (fgets(name, 256, f)) {
			name[strlen(name) -1] = '\0';
			if (name[0] == '\0') {
				continue;
			}
			fgets(val, 256, f);
			val[strlen(val) - 1] = '\0';
			if (!b && strcmp(name, key) == 0) {
				history("  found", val);
				b = true;
				strcpy(sval, val);
				continue;
			}
			fprintf(f2, "%s\n%s\n", name, val);
		}
		fclose(f2);
		fclose(f);
		if (b) {
			rename(fname(TMPFILE), fname(NRNBBS));
		}
	}else{
		b = false;
	}
	release_lock();
	return b;
}


bool nrnbbs_look(const char* key) {
	history("look", key);
	get_lock();
	bool b = false;
	FILE* f = fopen(fname(NRNBBS), "r");
	if (f != (FILE*)0) {
		char name[256], val[256];
		while (fgets(name, 256, f)) {
			name[strlen(name) - 1] = '\0';
			fgets(val, 256, f);
			if (strcmp(name, key) == 0) {
				history("  found");
				b = true;
				break;
			}
		}
		fclose(f);
	}else{
		b = false;
	}
	release_lock();
	return b;
}


void nrnbbs_exec(const char* cmd) {
	history("exec", cmd);
	char buf[256];
	sprintf(buf,"%s&", cmd);
	system(buf);
}


static
RETSIGTYPE
nrnbbs_handler(int) {
	long i;
#if defined(SIGNAL_CAST)
	signal(NOTIFY_SIGNAL, (SIGNAL_CAST)nrnbbs_handler);
#else
	signal(NOTIFY_SIGNAL, nrnbbs_handler);
#endif
	for (i = 0; i < cblist_->count(); ++i) {
		cblist_->item(i)->execute();
	}
}

void nrnbbs_notify(const char* key, NrnBBSCallback cb){
	history("nrnbbs_notify", key);
	if (!cblist_) {
		cblist_ = new NrnBBSCallbackList();
	}
	cblist_->append(new NrnBBSCallbackItem(key, cb));
	get_lock();
	FILE* f = fopen(fname(NOTIFY), "a");
	if (!f) {
		fprintf(stderr, "can't open %s\n", fname(NOTIFY));
		return;
	}
	fprintf(f, "%s\n%d\n", key, getpid());
	fclose(f);
	release_lock();
#if defined(SIGNAL_CAST)
	signal(NOTIFY_SIGNAL, (SIGNAL_CAST)nrnbbs_handler);
#else
	signal(NOTIFY_SIGNAL, nrnbbs_handler);
#endif
}

NrnBBSCallbackItem::NrnBBSCallbackItem(const char* s, NrnBBSCallback cb) {
	cb_ = cb;
	s_ = s;
}

NrnBBSCallbackItem::~NrnBBSCallbackItem() {}

bool NrnBBSCallbackItem::equal(const char* s) {
	return (strcmp(s_.string(), s) == 0) ? true : false;
}

void NrnBBSCallbackItem::execute() {
	char buf[256];
	sprintf(buf, "nrnbbs_notifying %s", s_.string());
	if (nrnbbs_take(buf)) {
		(*cb_)(s_.string());
	}
}
