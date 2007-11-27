#ifndef H_bbsconfig_included
#define H_bbsconfig_included 1

/* following are relevant to src/parallel implmentation of ParallelContext */
/* Define if PVM available */  
#undef HAVE_PVM3_H
/* Set to 1 if the standard template library exists */
#undef HAVE_STL
/* Set to 1 if SIGPOLL is a possible signal */
#undef HAVE_SIGPOLL
/* Set to 1 if can use the new PVM functions */
#undef HAVE_PKMESG 

#endif /* H_config_included */

