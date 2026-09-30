# MSVC (and clang-cl) have no system GNU readline. MinGW uses the MSYS2 package (setup.exe world).
# wineditline is not enough: it has readline() and add_history() but not rl_event_hook, which
# InterViews-while-prompt uses on the existing #ifdef READLINE path.
#
# vcpkg's Windows "readline" port is GNU readline 5.0 (xiaozhuai/readline-win32, GPL-2, same license
# as Unix GNU readline). Fetch that port and static-link it so rl_event_hook is a real data symbol
# (MSVC DLL data imports need dllimport; hoc.cpp uses extern "C" declarations, not readline.h).
#
# Do not add_subdirectory their CMakeLists.txt: it is cmake_minimum_required(VERSION 3.0), which
# CMake 4 rejects, and its PUBLIC include dir is the source root (leaks their config.h into NEURON).

include(FetchContent)

if(POLICY CMP0135)
  cmake_policy(SET CMP0135 NEW)
endif()

# Their doc/ has no CMakeLists.txt, so MakeAvailable populates without adding their project().
# <name>_SOURCE_DIR remains the source root.
FetchContent_Declare(
  nrn_readline_win32
  URL https://github.com/xiaozhuai/readline-win32/archive/0fa4001557c27157a51a9ca7f32a8c50bc97927a.tar.gz
  URL_HASH SHA256=2af1ddb990a9ae47e0bf260e1af083ede4fc2c50165617d6eb91e7758cb4f716
  SOURCE_SUBDIR doc)

FetchContent_MakeAvailable(nrn_readline_win32)

set(_nrn_rl_src "${nrn_readline_win32_SOURCE_DIR}")
set(_nrn_rl_srcs
    ${_nrn_rl_src}/readline.c
    ${_nrn_rl_src}/funmap.c
    ${_nrn_rl_src}/keymaps.c
    ${_nrn_rl_src}/vi_mode.c
    ${_nrn_rl_src}/parens.c
    ${_nrn_rl_src}/rltty.c
    ${_nrn_rl_src}/complete.c
    ${_nrn_rl_src}/bind.c
    ${_nrn_rl_src}/isearch.c
    ${_nrn_rl_src}/display.c
    ${_nrn_rl_src}/signals.c
    ${_nrn_rl_src}/util.c
    ${_nrn_rl_src}/kill.c
    ${_nrn_rl_src}/undo.c
    ${_nrn_rl_src}/macro.c
    ${_nrn_rl_src}/input.c
    ${_nrn_rl_src}/callback.c
    ${_nrn_rl_src}/terminal.c
    ${_nrn_rl_src}/xmalloc.c
    ${_nrn_rl_src}/history.c
    ${_nrn_rl_src}/histsearch.c
    ${_nrn_rl_src}/histexpand.c
    ${_nrn_rl_src}/histfile.c
    ${_nrn_rl_src}/nls.c
    ${_nrn_rl_src}/search.c
    ${_nrn_rl_src}/shell.c
    ${_nrn_rl_src}/savestring.c
    ${_nrn_rl_src}/tilde.c
    ${_nrn_rl_src}/text.c
    ${_nrn_rl_src}/misc.c
    ${_nrn_rl_src}/compat.c
    ${_nrn_rl_src}/mbutil.c
    ${_nrn_rl_src}/support/wcwidth.c)

add_library(nrn_readline STATIC ${_nrn_rl_srcs})
# PRIVATE: their config.h / readline.h live at the source root.
target_include_directories(nrn_readline PRIVATE "${_nrn_rl_src}")
# READLINE_LIBRARY must be on the command line: tilde.c includes tilde.h before any #define, and
# that header uses <readline/rlstdc.h> otherwise. Some .c files also #define it (C4005).
target_compile_definitions(nrn_readline PRIVATE READLINE_LIBRARY HAVE_CONFIG_H
                                                _CRT_SECURE_NO_WARNINGS READLINE_STATIC)
if(CMAKE_C_COMPILER_ID STREQUAL "MSVC")
  target_compile_options(nrn_readline PRIVATE /wd4005)
endif()

set(Readline_LIBRARY nrn_readline)
set(Readline_INCLUDE_DIR "${_nrn_rl_src}")
set(READLINE_FOUND TRUE)
set(NRN_READLINE_FETCHED TRUE)
# GNU readline 5.0 defines rl_event_hook; do not grep a CMake target.
set(NRN_READLINE_HAS_EVENT_HOOK TRUE)

unset(_nrn_rl_src)
unset(_nrn_rl_srcs)

message(STATUS "Readline not found; using fetched GNU readline 5.0 (win32 static)")
