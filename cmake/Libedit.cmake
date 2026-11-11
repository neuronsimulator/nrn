# Shipped binaries static-link BSD libedit (NetBSD editline) instead of GNU readline. Pin:
# larkmjc/libedit f3a98319002d0eca05e2a370a68b28a2afa6a822 ("trim termcap file to xterm, tmux, and
# screen").
#
# MSVC has no termcap. MinGW has no termios.h, so it cannot compile the Unix libedit sources. Both
# build this port's Win32 console stack (regex, OpenSSH-derived compat, PDCursesMod, embedded
# terminfo). That stack's headers declare mkstemp, fseeko, and the passwd helpers without defining
# them, and its misc.c also defines dlopen, which nrniv already provides. Unix wheels link a static
# tinfo or ncurses. Do not add_subdirectory upstream CMakeLists.txt: it installs static libraries
# into the NEURON prefix and builds a demo.

include(FetchContent)

if(POLICY CMP0135)
  cmake_policy(SET CMP0135 NEW)
endif()

set(NRN_LIBEDIT_GIT_SHA "f3a98319002d0eca05e2a370a68b28a2afa6a822")
set(NRN_LIBEDIT_URL "https://github.com/larkmjc/libedit/archive/${NRN_LIBEDIT_GIT_SHA}.tar.gz")
set(NRN_LIBEDIT_SHA256 "a9767395f3decc84fa26d7727795e165da9e2d8df1ef2120ecd6059dd9c471ec")

# libedit/ has no CMakeLists.txt, so MakeAvailable populates without their project().
FetchContent_Declare(
  nrn_libedit
  URL ${NRN_LIBEDIT_URL}
  URL_HASH SHA256=${NRN_LIBEDIT_SHA256}
  SOURCE_SUBDIR libedit)

FetchContent_MakeAvailable(nrn_libedit)

set(_src "${nrn_libedit_SOURCE_DIR}")

# hoc assigns rl_getc_function before each readline(). Upstream copies a non-NULL pointer into
# EL_GETCFN only inside rl_initialize, which also wipes history.
function(_nrn_libedit_replace file marker old new)
  file(READ "${file}" _text)
  # MinGW CMake file(WRITE) stores the patched header with CRLF. Needles are LF.
  string(REPLACE "\r\n" "\n" _text "${_text}")
  string(FIND "${_text}" "${marker}" _mark)
  if(NOT _mark EQUAL -1)
    return()
  endif()
  string(FIND "${_text}" "${old}" _pos)
  if(_pos EQUAL -1)
    message(FATAL_ERROR "libedit patch missed ${file}")
  endif()
  string(REPLACE "${old}" "${new}" _text "${_text}")
  file(WRITE "${file}" "${_text}")
endfunction()

_nrn_libedit_replace(
  "${_src}/libedit/readline.c"
  "NEURON: hoc assigns rl_getc_function"
  [=[	if (rl_event_hook && !(e->el_flags & NO_TTY)) {
		el_set(e, EL_GETCFN, _rl_event_read_char);
		used_event_hook = 1;
	}

	if (!rl_event_hook && used_event_hook) {
		el_set(e, EL_GETCFN, EL_BUILTIN_GETCFN);
		used_event_hook = 0;
	}
]=]
  [=[	/* NEURON: hoc assigns rl_getc_function before each readline().
	   rl_initialize copies a non-NULL pointer into EL_GETCFN only once.
	   Refresh it on every call. NEURON does not set rl_event_hook. */
	if (rl_event_hook && !(e->el_flags & NO_TTY)) {
		el_set(e, EL_GETCFN, _rl_event_read_char);
		used_event_hook = 1;
	} else if (rl_getc_function) {
		el_set(e, EL_GETCFN, _getc_function);
		used_event_hook = 0;
	} else {
		el_set(e, EL_GETCFN, EL_BUILTIN_GETCFN);
		used_event_hook = 0;
	}
]=])

# strlcpy.c is empty unless WIN32 or __linux__. MinGW has neither macro.
_nrn_libedit_replace(
  "${_src}/libedit/strlcpy.c" "defined __MINGW32__" "#if defined WIN32 || defined __linux__"
  "#if defined WIN32 || defined __linux__ || defined __MINGW32__")

# gcc has no _BitScanForward. MinGW only takes this branch when the target defines WIN32.
_nrn_libedit_replace(
  "${_src}/libedit/tty.c"
  "NEURON: gcc has no _BitScanForward"
  [=[#ifdef WIN32
#define isatty _isatty
#include <intrin.h>
static __forceinline int ffs(int x)
{
	unsigned long i;
	if (_BitScanForward(&i, x))
		return (i + 1);
	return (0);
}
]=]
  [=[#ifdef WIN32
#define isatty _isatty
#if defined(_MSC_VER)
#include <intrin.h>
static __forceinline int ffs(int x)
{
	unsigned long i;
	if (_BitScanForward(&i, x))
		return (i + 1);
	return (0);
}
#else
/* NEURON: gcc has no _BitScanForward. */
static inline int ffs(int x)
{
	return __builtin_ffs(x);
}
#endif
]=])

# nrniv already links src/mswin/dlfcn.c. Leave the port's copies out of nrn_edit_compat.
_nrn_libedit_replace(
  "${_src}/compat/win32/misc.c" "NRN_LIBEDIT_SKIP_DLFCN"
  "static DWORD last_dlerror = ERROR_SUCCESS;" "#ifndef NRN_LIBEDIT_SKIP_DLFCN
static DWORD last_dlerror = ERROR_SUCCESS;")
_nrn_libedit_replace(
  "${_src}/compat/win32/misc.c"
  "NEURON: nrniv provides dlopen"
  [=[	return "Failed to format error message";
}

/*fopen on Windows]=]
  [=[	return "Failed to format error message";
}
#endif /* NRN_LIBEDIT_SKIP_DLFCN */

/* NEURON: nrniv provides dlopen. */
/*fopen on Windows]=])

# MinGW's C library already defines off_t (and pid_t). The shim's long long off_t does not match.
# MSVC's ucrt header sets _OFF_T_DEFINED without typedef'ing off_t when __STDC__ is 1, which this
# shim forces, so the guard has to be __MINGW32__ rather than _OFF_T_DEFINED. The marker has to be a
# substring of the replacement. "has off_t" is not, because the comment says "has dev_t, off_t".
_nrn_libedit_replace(
  "${_src}/compat/win32/include/sys/types.h"
  "NEURON: MinGW already has dev_t, off_t, mode_t, and pid_t."
  [=[typedef _dev_t dev_t;
typedef long long off_t;
typedef unsigned int uid_t;
typedef unsigned int gid_t;

typedef unsigned short _mode_t;
typedef _mode_t mode_t;
typedef int ssize_t;
typedef int pid_t;
]=]
  [=[/* NEURON: MinGW already has dev_t, off_t, mode_t, and pid_t. */
#ifndef __MINGW32__
typedef _dev_t dev_t;
typedef long long off_t;
typedef unsigned short _mode_t;
typedef _mode_t mode_t;
typedef int pid_t;
#endif
typedef unsigned int uid_t;
typedef unsigned int gid_t;
/* NEURON: MinGW ssize_t is long long, from corecrt.h. */
#ifndef __MINGW32__
typedef int ssize_t;
#endif
]=])
_nrn_libedit_replace(
  "${_src}/compat/win32/include/stdio.h"
  "NEURON: MinGW stdio.h already defines off_t"
  [=[typedef long long off_t;
int fseeko(FILE *stream, off_t offset, int whence);
off_t ftello(FILE *stream);

/* required by el.c */
typedef int ssize_t;
]=]
  [=[/* NEURON: MinGW stdio.h already defines off_t and declares fseeko. */
#ifndef __MINGW32__
typedef long long off_t;
#endif
int fseeko(FILE *stream, off_t offset, int whence);
off_t ftello(FILE *stream);

/* required by el.c */
/* NEURON: MinGW ssize_t is long long, from corecrt.h. */
#ifndef __MINGW32__
#ifndef _SSIZE_T_DEFINED
typedef int ssize_t;
#define _SSIZE_T_DEFINED
#endif
#endif
]=])

# Trees configured before the ssize_t guard still have the unguarded typedef. A clean fetch already
# contains the marker, so this second replace does not run.
_nrn_libedit_replace(
  "${_src}/compat/win32/include/sys/types.h"
  "NEURON: MinGW ssize_t is long long"
  [=[typedef unsigned int uid_t;
typedef unsigned int gid_t;
typedef int ssize_t;
]=]
  [=[typedef unsigned int uid_t;
typedef unsigned int gid_t;
/* NEURON: MinGW ssize_t is long long, from corecrt.h. */
#ifndef __MINGW32__
typedef int ssize_t;
#endif
]=])
_nrn_libedit_replace(
  "${_src}/compat/win32/include/stdio.h"
  "NEURON: MinGW ssize_t is long long"
  [=[/* required by el.c */
#ifndef _SSIZE_T_DEFINED
typedef int ssize_t;
#define _SSIZE_T_DEFINED
#endif
]=]
  [=[/* required by el.c */
/* NEURON: MinGW ssize_t is long long, from corecrt.h. */
#ifndef __MINGW32__
#ifndef _SSIZE_T_DEFINED
typedef int ssize_t;
#define _SSIZE_T_DEFINED
#endif
#endif
]=])

# Forward declarations in w32fd.c are non-static and the definitions are static.
_nrn_libedit_replace(
  "${_src}/compat/win32/w32fd.c"
  "NEURON: gcc rejects a static definition after a non-static declaration"
  [=[void fd_table_set(struct w32_io* pio, int index);

void fd_decode_state(char*);
]=]
  [=[/* NEURON: gcc rejects a static definition after a non-static declaration. */
static void fd_table_set(struct w32_io* pio, int index);

static void fd_decode_state(char*);
]=])
# mode_t is a short. va_arg must take the promoted type.
_nrn_libedit_replace(
  "${_src}/compat/win32/w32fd.c" "NEURON: mode_t is promoted to int"
  [=[mode = va_arg(valist, mode_t);]=]
  [=[mode = (mode_t)va_arg(valist, int); /* NEURON: mode_t is promoted to int. */]=])

# processBuffer's buffer argument is unsigned char**. gcc rejects char**.
_nrn_libedit_replace(
  "${_src}/compat/win32/termio.c"
  "NEURON: processBuffer writes an unsigned char buffer"
  [=[	char *respbuf = NULL;
	size_t resplen = 0;
	debug5("WriteThread thread, io:%p", pio);
]=]
  [=[	/* NEURON: processBuffer writes an unsigned char buffer. */
	unsigned char *respbuf = NULL;
	size_t resplen = 0;
	debug5("WriteThread thread, io:%p", pio);
]=])

# signal.c includes one UCRT signal.h by absolute path. MinGW gcc cannot parse that header.
_nrn_libedit_replace(
  "${_src}/compat/win32/signal.c"
  "NEURON: MinGW cannot parse the UCRT signal.h"
  [=[//#include <signal.h>
#include "C:\\Program Files (x86)\\Windows Kits\\10\\Include\\10.0.22621.0\\ucrt\\signal.h"
]=]
  [=[//#include <signal.h>
#ifdef __MINGW32__
/* NEURON: MinGW cannot parse the UCRT signal.h. crtheaders.h defines SIGNAL_H. */
#include SIGNAL_H
#else
#include "C:\\Program Files (x86)\\Windows Kits\\10\\Include\\10.0.22621.0\\ucrt\\signal.h"
#endif
]=])

# gcc rejects int* where PeekConsoleInputW and ReadConsoleInputW want LPDWORD.
_nrn_libedit_replace(
  "${_src}/compat/win32/console.c"
  "NEURON: PeekConsoleInputW wants LPDWORD"
  [=[	INPUT_RECORD peek_input;
	int out_count = 0;
]=]
  [=[	INPUT_RECORD peek_input;
	/* NEURON: PeekConsoleInputW wants LPDWORD, and gcc rejects int*. */
	DWORD out_count = 0;
]=])

# MinGW direct.h declares 1-arg mkdir. Include it before the rename macro, and define struct
# w32_stat before any prototype. A struct tag first seen in a parameter list is a different type on
# gcc.
_nrn_libedit_replace(
  "${_src}/compat/win32/include/sys/stat.h"
  "NEURON: direct.h declares 1-arg mkdir"
  [=[#ifndef W32_COMPAT_IMPL
#define fstat(a,b)	w32_fstat((a), (b))
#define stat w32_stat
#define lstat w32_lstat
#define mkdir w32_mkdir
#define chmod w32_chmod
#define fchmod w32_fchmod
#endif

int w32_fstat(int fd, struct w32_stat *buf);
int w32_stat(const char *path, struct w32_stat *buf);
int w32_lstat(const char *path, struct w32_stat *buf);
int w32_mkdir(const char *pathname, unsigned short mode);
int w32_chmod(const char *, mode_t);
int w32_fchmod(int fd, mode_t mode);

struct w32_stat {
	dev_t     st_dev;     /* ID of device containing file */
	unsigned short     st_ino;     /* inode number */
	unsigned short    st_mode;    /* protection */
	short    st_nlink;   /* number of hard links */
	short     st_uid;     /* user ID of owner */
	short     st_gid;     /* group ID of owner */
	dev_t     st_rdev;    /* device ID (if special file) */
	__int64     st_size;    /* total size, in bytes */
	__int64    st_atime;   /* time of last access */
	__int64    st_mtime;   /* time of last modification */
	__int64    st_ctime;   /* time of last status change */
};
]=]
  [=[/* NEURON: direct.h declares 1-arg mkdir. Include it before the rename macro. */
#include <direct.h>
#ifndef W32_COMPAT_IMPL
#define fstat(a,b)	w32_fstat((a), (b))
#define stat w32_stat
#define lstat w32_lstat
#define mkdir w32_mkdir
#define chmod w32_chmod
#define fchmod w32_fchmod
#endif

/* NEURON: gcc gives a prototype-scope struct w32_stat its own type. */
struct w32_stat {
	dev_t     st_dev;     /* ID of device containing file */
	unsigned short     st_ino;     /* inode number */
	unsigned short    st_mode;    /* protection */
	short    st_nlink;   /* number of hard links */
	short     st_uid;     /* user ID of owner */
	short     st_gid;     /* group ID of owner */
	dev_t     st_rdev;    /* device ID (if special file) */
	__int64     st_size;    /* total size, in bytes */
	__int64    st_atime;   /* time of last access */
	__int64    st_mtime;   /* time of last modification */
	__int64    st_ctime;   /* time of last status change */
};

int w32_fstat(int fd, struct w32_stat *buf);
int w32_stat(const char *path, struct w32_stat *buf);
int w32_lstat(const char *path, struct w32_stat *buf);
int w32_mkdir(const char *pathname, unsigned short mode);
int w32_chmod(const char *, mode_t);
int w32_fchmod(int fd, mode_t mode);
]=])

# pthread_time.h defines nanosleep, and libc provides usleep and gettimeofday.
_nrn_libedit_replace(
  "${_src}/compat/win32/misc.c"
  "NEURON: MinGW's pthread_time.h already defines nanosleep"
  [=[int
usleep(unsigned int useconds)
{
	Sleep(useconds / 1000);
	return 1;
}

static LONGLONG
timespec_to_nsec(const struct timespec *req)
{
	LONGLONG sec = req->tv_sec;
	return sec * NSEC_IN_SEC + req->tv_nsec;
}


int
nanosleep(const struct timespec *req, struct timespec *rem)
]=]
  [=[#ifndef __MINGW32__
/* NEURON: MinGW's pthread_time.h already defines nanosleep. */
int
usleep(unsigned int useconds)
{
	Sleep(useconds / 1000);
	return 1;
}

static LONGLONG
timespec_to_nsec(const struct timespec *req)
{
	LONGLONG sec = req->tv_sec;
	return sec * NSEC_IN_SEC + req->tv_nsec;
}


int
nanosleep(const struct timespec *req, struct timespec *rem)
]=])
_nrn_libedit_replace(
  "${_src}/compat/win32/misc.c"
  "NEURON: MinGW libc provides usleep and nanosleep"
  [=[		errno = EFAULT;
		return -1;
	}
}

/* This routine is contributed by  * Author: NoMachine <developers@nomachine.com>
]=]
  [=[		errno = EFAULT;
		return -1;
	}
}
#endif /* NEURON: MinGW libc provides usleep and nanosleep. */

/* This routine is contributed by  * Author: NoMachine <developers@nomachine.com>
]=])
_nrn_libedit_replace(
  "${_src}/compat/win32/misc.c"
  "NEURON: MinGW libc provides gettimeofday"
  [=[static int
gettimeofday(struct timeval *tv, void *tz)
]=]
  [=[#ifndef __MINGW32__
/* NEURON: MinGW libc provides gettimeofday. */
int
gettimeofday(struct timeval *tv, void *tz)
]=])
_nrn_libedit_replace(
  "${_src}/compat/win32/misc.c"
  "NEURON: end of the MSVC gettimeofday definition"
  [=[	return 0;
}

static void
explicit_bzero(void *b, size_t len)
]=]
  [=[	return 0;
}
#endif /* NEURON: end of the MSVC gettimeofday definition. */

static void
explicit_bzero(void *b, size_t len)
]=])

_nrn_libedit_replace(
  "${_src}/compat/win32/misc_internal.h"
  "NEURON: MSVC errno.h defines EOTHER"
  [=[#include <Windows.h>
#include <VersionHelpers.h>
]=]
  [=[#include <Windows.h>
#include <VersionHelpers.h>
#include <errno.h>
#include <wctype.h>
/* NEURON: MSVC errno.h defines EOTHER. MinGW does not. */
#ifndef EOTHER
#define EOTHER EIO
#endif
/* NEURON: __ascii_iswalpha is an MSVC CRT internal. */
#ifndef __ascii_iswalpha
#define __ascii_iswalpha(c) iswalpha((wint_t)(c))
#endif
]=])

set(_edit_sources
    chared.c
    chartype.c
    common.c
    el.c
    eln.c
    emacs.c
    filecomplete.c
    hist.c
    history.c
    historyn.c
    keymacro.c
    literal.c
    map.c
    parse.c
    prompt.c
    read.c
    readline.c
    refresh.c
    search.c
    sig.c
    strlcpy.c
    terminal.c
    tokenizer.c
    tokenizern.c
    tty.c
    unvis.c
    vi.c
    vis.c)
list(TRANSFORM _edit_sources PREPEND "${_src}/libedit/")

function(_nrn_libedit_mark target)
  set_target_properties(
    ${target}
    PROPERTIES POSITION_INDEPENDENT_CODE ON
               C_STANDARD 11
               C_STANDARD_REQUIRED ON)
endfunction()

set(NRN_LIBEDIT_PDCURSES_README "")
set(NRN_LIBEDIT_TERMCAP_NOTICE "")

if(WIN32)
  set(_ucrt "")
  if(MSVC)
    foreach(_dir IN LISTS CMAKE_C_IMPLICIT_INCLUDE_DIRECTORIES
                          CMAKE_CXX_IMPLICIT_INCLUDE_DIRECTORIES)
      if(_dir MATCHES "[\\\\/]ucrt$")
        set(_ucrt "${_dir}")
        break()
      endif()
    endforeach()
    if(NOT _ucrt)
      set(_programfilesx86 "ProgramFiles(x86)")
      set(_kit_roots "$ENV{CMAKE_WINDOWS_KITS_10_DIR}" "$ENV{ProgramFiles}/Windows Kits/10"
                     "$ENV{${_programfilesx86}}/Windows Kits/10")
      foreach(_root IN LISTS _kit_roots)
        if(NOT _root OR NOT EXISTS "${_root}/Include")
          continue()
        endif()
        file(
          GLOB _vers
          LIST_DIRECTORIES true
          "${_root}/Include/10.*")
        if(_vers)
          list(
            SORT _vers
            COMPARE NATURAL
            ORDER DESCENDING)
          list(GET _vers 0 _verdir)
          if(EXISTS "${_verdir}/ucrt/stdio.h")
            set(_ucrt "${_verdir}/ucrt")
            break()
          endif()
        endif()
      endforeach()
    endif()
  else()
    # MinGW: the shims include the compiler's C headers by full path. Do not use the msys
    # /usr/include/termios.h; that pulls in msys-2.0.dll, which the installer does not ship.
    foreach(_dir IN LISTS CMAKE_C_IMPLICIT_INCLUDE_DIRECTORIES)
      if(EXISTS "${_dir}/stdio.h" AND EXISTS "${_dir}/sys/types.h")
        set(_ucrt "${_dir}")
        break()
      endif()
    endforeach()
  endif()
  if(NOT _ucrt OR NOT EXISTS "${_ucrt}/stdio.h")
    message(
      FATAL_ERROR
        "libedit: C runtime headers not found. The Win32 shims include those headers by full path.")
  endif()
  set(_hdrdir "${CMAKE_CURRENT_BINARY_DIR}/libedit-include")
  file(MAKE_DIRECTORY "${_hdrdir}")
  set(_crtheaders "${_hdrdir}/crtheaders.h")
  file(WRITE "${_crtheaders}" "#pragma once\n")
  if(MSVC)
    file(
      GLOB_RECURSE _uchdrs
      RELATIVE "${_ucrt}"
      "${_ucrt}/*.h")
  else()
    # Only the headers the shims include by macro. A full mingw include tree defines macros such as
    # WINDOWS_H that collide with header guards.
    set(_uchdrs
        ctype.h
        fcntl.h
        stddef.h
        stdio.h
        stdlib.h
        string.h
        time.h
        signal.h
        sys/stat.h
        sys/types.h)
  endif()
  foreach(_hdr IN LISTS _uchdrs)
    string(TOUPPER "${_hdr}" _macro)
    string(REGEX REPLACE "[/\\\\.]" "_" _macro "${_macro}")
    if(NOT _macro MATCHES "^[A-Z0-9_]+$")
      continue()
    endif()
    if(NOT EXISTS "${_ucrt}/${_hdr}")
      message(FATAL_ERROR "libedit: missing C runtime header ${_ucrt}/${_hdr}")
    endif()
    file(TO_CMAKE_PATH "${_ucrt}/${_hdr}" _full)
    file(APPEND "${_crtheaders}" "#define ${_macro} \"${_full}\"\n")
  endforeach()

  # Embed data/termcap. Upstream opens the relative path "data/termcap", which a wheel does not have
  # as its cwd. The file's 4-clause notice is installed with the binary; it is not in the top-level
  # LICENSE.
  _nrn_libedit_replace(
    "${_src}/compat/terminfo/tparser.h"
    "int tc_feed("
    [=[int tc_read(tc_parser *tp, const char *filesymbol);]=]
    [=[int tc_feed(tc_parser *tp, const char *bytes, size_t n);
int tc_read(tc_parser *tp, const char *filesymbol);]=])
  _nrn_libedit_replace(
    "${_src}/compat/terminfo/tparser.c"
    "int tc_feed("
    [=[int tc_read(tc_parser *tp, const char *filesymbol)]=]
    [=[int tc_feed(tc_parser *tp, const char *bytes, size_t n)
{
	size_t i;

	tp->ln = 1, tp->col = 0;
	for (i = 0; i < n; i++) {
		if (tc_parse(tp, bytes[i]) < 0)
			return -1;
	}
	return 0;
}

int tc_read(tc_parser *tp, const char *filesymbol)]=])
  _nrn_libedit_replace(
    "${_src}/compat/terminfo/termcap.c"
    "NEURON: the termcap database is embedded"
    [=[static const char *termcap_file = "data/termcap";
static tc_symbol sym_tc;
static lhmap cap_map;

static int tinfo_init()
{
	if (initialized) return 0;
	if (tc_init(&tp, opt_debug) < 0) return -1;
	if (tc_read(&tp, termcap_file) < 0) return -1;]=]
    [=[/* NEURON: the termcap database is embedded; do not fopen data/termcap. */
extern const unsigned char nrn_libedit_termcap[];
extern const unsigned int nrn_libedit_termcap_len;
static tc_symbol sym_tc;
static lhmap cap_map;

static int tinfo_init()
{
	if (initialized) return 0;
	if (tc_init(&tp, opt_debug) < 0) return -1;
	if (tc_feed(&tp, (const char *)nrn_libedit_termcap, nrn_libedit_termcap_len) < 0)
		return -1;]=])

  set(_termcap_c "${CMAKE_CURRENT_BINARY_DIR}/libedit/nrn_libedit_termcap.c")
  file(READ "${_src}/data/termcap" _hex HEX)
  string(REGEX REPLACE "(..)" "0x\\1," _bytes "${_hex}")
  string(LENGTH "${_bytes}" _nbytes)
  set(_wrapped "")
  set(_i 0)
  while(_i LESS _nbytes)
    string(SUBSTRING "${_bytes}" ${_i} 80 _chunk)
    string(APPEND _wrapped "${_chunk}\n")
    math(EXPR _i "${_i} + 80")
  endwhile()
  file(
    WRITE "${_termcap_c}"
    "/* Generated from libedit data/termcap. Its copyright notice is installed beside the binary. */\n"
    "const unsigned char nrn_libedit_termcap[] = {\n"
    "${_wrapped}"
    "0x0a\n"
    "};\n"
    "const unsigned int nrn_libedit_termcap_len = sizeof(nrn_libedit_termcap);\n")

  set(_win_includes "${_hdrdir}" "${_src}/compat/win32/include" "${_src}/compat/regex/include")

  set(_regex_sources regcomp.c regerror.c regexec.c regfree.c)
  list(TRANSFORM _regex_sources PREPEND "${_src}/compat/regex/")
  add_library(nrn_edit_regex STATIC ${_regex_sources})
  target_include_directories(nrn_edit_regex PRIVATE ${_win_includes})

  file(GLOB _compat_sources "${_src}/compat/win32/*.c")
  add_library(nrn_edit_compat STATIC ${_compat_sources}
                                     "${CMAKE_CURRENT_LIST_DIR}/libedit_win32_missing.c")
  target_include_directories(nrn_edit_compat PRIVATE ${_win_includes})

  file(GLOB _pdc_sources "${_src}/compat/pdcurses/*.c" "${_src}/compat/pdcurses/vt/*.c")
  add_library(nrn_edit_pdcurses STATIC ${_pdc_sources})
  target_include_directories(nrn_edit_pdcurses PRIVATE ${_win_includes}
                                                       "${_src}/compat/pdcurses/include")

  file(GLOB _tinfo_sources "${_src}/compat/terminfo/*.c")
  add_library(nrn_edit_terminfo STATIC ${_tinfo_sources} "${_termcap_c}")
  target_include_directories(nrn_edit_terminfo PRIVATE ${_win_includes} "${_src}/compat/terminfo")

  foreach(_tgt nrn_edit_regex nrn_edit_compat nrn_edit_pdcurses nrn_edit_terminfo)
    _nrn_libedit_mark(${_tgt})
    # MinGW does not define WIN32. The port's sources and the checked-in libedit headers key on it.
    # Keep the macro on these targets; defining it for all of NEURON would change other code.
    target_compile_definitions(${_tgt} PRIVATE WIN32)
    if(MSVC)
      target_compile_definitions(${_tgt} PRIVATE _CRT_SECURE_NO_WARNINGS)
    endif()
  endforeach()
  target_compile_definitions(nrn_edit_compat PRIVATE NRN_LIBEDIT_SKIP_DLFCN)

  add_library(nrn_edit STATIC ${_edit_sources})
  # gcc 14 makes the OpenSSH port's char* / unsigned char* mismatches errors. MSVC accepts them.
  if(NOT MSVC)
    foreach(_tgt nrn_edit_regex nrn_edit_compat nrn_edit_pdcurses nrn_edit_terminfo nrn_edit)
      target_compile_options(${_tgt} PRIVATE -Wno-error=incompatible-pointer-types
                                             -Wno-error=int-conversion)
    endforeach()
  endif()
  target_compile_definitions(nrn_edit PRIVATE WIN32)
  target_include_directories(nrn_edit PRIVATE ${_win_includes} "${_src}/libedit/include"
                                              "${_src}/libedit")
  # PUBLIC so a static library's system libraries reach nrniv_lib. PRIVATE usage requirements of a
  # static library are not applied to the consumer.
  target_link_libraries(
    nrn_edit
    PUBLIC nrn_edit_regex
           nrn_edit_compat
           nrn_edit_pdcurses
           nrn_edit_terminfo
           crypt32
           secur32
           shlwapi
           userenv
           wsock32
           ws2_32)
  set(NRN_LIBEDIT_PDCURSES_README "${_src}/compat/pdcurses/README.md")
  set(NRN_LIBEDIT_TERMCAP_NOTICE "${_src}/data/termcap")
else()
  # Unix wheels only. WIN32 took the console stack above. Do not also search libreadline.
  unset(NRN_LIBEDIT_TERMCAP CACHE)
  set(_saved_suffixes "${CMAKE_FIND_LIBRARY_SUFFIXES}")
  if(NOT WIN32)
    set(CMAKE_FIND_LIBRARY_SUFFIXES ".a")
  endif()
  set(_termcap_hints ${CMAKE_PREFIX_PATH})
  if(_termcap_hints)
    # PATHS are install prefixes. find_library does not look in prefix/lib unless PATH_SUFFIXES says
    # so, and NO_DEFAULT_PATH skips the CMAKE_PREFIX_PATH lib search.
    find_library(
      NRN_LIBEDIT_TERMCAP
      NAMES tinfo ncurses
      PATHS ${_termcap_hints}
      PATH_SUFFIXES lib
      NO_DEFAULT_PATH)
  endif()
  # A missed find_library caches <VAR>-NOTFOUND and a later call will not search again.
  if(NOT NRN_LIBEDIT_TERMCAP)
    unset(NRN_LIBEDIT_TERMCAP CACHE)
    find_library(NRN_LIBEDIT_TERMCAP NAMES tinfo ncurses ncursesw)
  endif()
  set(CMAKE_FIND_LIBRARY_SUFFIXES "${_saved_suffixes}")
  if(NOT NRN_LIBEDIT_TERMCAP)
    message(FATAL_ERROR "libedit requires tinfo or ncurses (static .a on Unix wheels).")
  endif()
  if(NOT WIN32)
    if(NOT NRN_LIBEDIT_TERMCAP MATCHES "\\.a$")
      message(FATAL_ERROR "Unix libedit must link a static termcap, found ${NRN_LIBEDIT_TERMCAP}")
    endif()
  endif()

  add_library(nrn_edit STATIC ${_edit_sources})
  target_include_directories(nrn_edit PRIVATE "${_src}/libedit/include" "${_src}/libedit")
  target_link_libraries(nrn_edit PUBLIC "${NRN_LIBEDIT_TERMCAP}")
endif()

_nrn_libedit_mark(nrn_edit)
if(MSVC)
  target_compile_definitions(nrn_edit PRIVATE _CRT_SECURE_NO_WARNINGS)
endif()

# Shadow a cached find_package(Readline) result such as the old nrn_readline target.
unset(Readline_LIBRARY CACHE)
unset(Readline_INCLUDE_DIR CACHE)
unset(READLINE_FOUND CACHE)
set(Readline_LIBRARY nrn_edit)
set(Readline_INCLUDE_DIR "")
set(READLINE_FOUND TRUE)
set(NRN_LIBEDIT_BUILTIN TRUE)
set(NRN_LIBEDIT_LICENSE_FILE "${_src}/LICENSE")

message(STATUS "libedit       | ${NRN_LIBEDIT_GIT_SHA} (${Readline_LIBRARY})")

unset(_src)
unset(_edit_sources)
