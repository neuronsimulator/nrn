#ifndef profile_h

#ifdef __cplusplus
extern "C" {
#endif
extern void start_profile(int);
extern void add_profile(int);
#ifdef __cplusplus
}
#endif

#if defined(PROFILE) && PROFILE > 0
#define PSTART(i) start_profile(i);
#define PSTOP(i) add_profile(i);
#else
#define PSTART(i) /**/
#define PSTOP(i) /**/
#endif

#endif
