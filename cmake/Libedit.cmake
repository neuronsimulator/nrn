# Shipped binaries static-link BSD libedit (NetBSD editline) instead of GNU readline. Pin:
# larkmjc/libedit f3a98319002d0eca05e2a370a68b28a2afa6a822 ("trim termcap file to xterm, tmux, and
# screen").
#
# MSVC has no termcap, so it builds this port's Win32 console stack (regex, OpenSSH-derived compat,
# PDCursesMod, embedded terminfo). MinGW gcc does not define the WIN32 macro that stack keys on, and
# the stack uses MSVC intrinsics, so MinGW builds the Unix sources and links MSYS2 ncurses. Unix
# wheels do the same and link a static tinfo or ncurses. Do not add_subdirectory upstream
# CMakeLists.txt: it installs static libraries into the NEURON prefix and builds a demo.

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

if(MSVC)
  set(_ucrt "")
  foreach(_dir IN LISTS CMAKE_C_IMPLICIT_INCLUDE_DIRECTORIES CMAKE_CXX_IMPLICIT_INCLUDE_DIRECTORIES)
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
  if(NOT _ucrt OR NOT EXISTS "${_ucrt}/stdio.h")
    message(
      FATAL_ERROR
        "libedit: Windows SDK UCRT headers not found. The Win32 shims include those headers by full path."
    )
  endif()
  set(_hdrdir "${CMAKE_CURRENT_BINARY_DIR}/libedit-include")
  file(MAKE_DIRECTORY "${_hdrdir}")
  set(_crtheaders "${_hdrdir}/crtheaders.h")
  file(WRITE "${_crtheaders}" "#pragma once\n")
  file(
    GLOB_RECURSE _uchdrs
    RELATIVE "${_ucrt}"
    "${_ucrt}/*.h")
  foreach(_hdr IN LISTS _uchdrs)
    string(TOUPPER "${_hdr}" _macro)
    string(REGEX REPLACE "[/\\\\.]" "_" _macro "${_macro}")
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
  add_library(nrn_edit_compat STATIC ${_compat_sources})
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
    target_compile_definitions(${_tgt} PRIVATE _CRT_SECURE_NO_WARNINGS)
  endforeach()

  add_library(nrn_edit STATIC ${_edit_sources})
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
  # Static on Unix wheels. MinGW keeps the shared MSYS2 import library so the installer can copy
  # ncurses DLLs. Do not also search libreadline.
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
