#ifndef nrnbbs_h
#define nrnbbs_h

#ifndef boolean
#define boolean int
#endif

typedef void (*NrnBBSCallback)(const char*);

#if defined(__cplusplus)
extern "C" {
#endif
boolean nrnbbs_connect();
void nrnbbs_disconnect();
boolean nrnbbs_connected();

void nrnbbs_post(const char*);
void nrnbbs_post_int(const char*, int);
void nrnbbs_post_string(const char*, const char*);

boolean nrnbbs_take(const char*);
boolean nrnbbs_take_int(const char*, int*);
boolean nrnbbs_take_string(const char*, char*);

boolean nrnbbs_look(const char*);

void nrnbbs_exec(const char*);

void nrnbbs_notify(const char*, NrnBBSCallback);

// return when *pflag = true or one step wait if no arg
void nrnbbs_wait(boolean* pflag = (boolean*)0);

/* for debugging and bbs management */
#if 0
void nrnbbs_clean();
void nnrbbs_quit();
int nrnbbs_count();
boolean nrnbbs_query(long index);
#endif

#if defined(__cplusplus)
}
#endif
#endif
