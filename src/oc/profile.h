#ifndef profile_h

extern void start_profile(int);
extern void add_profile(int);

#if defined(PROFILE) && PROFILE > 0
#define PSTART(i) start_profile(i);
#define PSTOP(i) add_profile(i);
#else
#define PSTART(i) /**/
#define PSTOP(i) /**/
#endif

#endif
