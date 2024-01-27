/* auto include file for metrowerks codewarrior for all nrn */
#include <Win32Headers.mch>
#pragma once off
#ifndef __WIN32__
#define __WIN32__ 1
#endif
#ifndef WIN32
#define WIN32 1
#endif
#define _WIN32
#ifndef _Windows
#define _Windows 1
#endif
struct Section;
struct Object;
struct Symbol;
#define motif_kit
#define sgi_motif_kit
#define printf  myprintf
#define vprintf myvprintf
#define gets    mygets
#define puts    myputs
#define fprintf myfprintf
#undef small
#undef near
#define small mysmall
#define near  mynear

#define stricmp _stricmp
#define putenv  _putenv

//#define system mysystem
#undef DELETE
#undef IGNORE
#define HOC        1
#define OOP        1
#define OC_CLASSES "nrnclass.h"
#define USECVODE   1
#define USEMATRIX  1
