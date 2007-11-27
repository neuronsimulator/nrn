
dnl This was ripped out of the readline-4.0 distribution.  It defines the
dnl variable TERMCAP_LIB, which tells us what to link memacs with.
dnl

AC_DEFUN([AC_NRN_READLINE],[

dnl decide whether we want to disable all terminal capabilities
if test "$with_memacs" = "" ; then
	with_memacs=yes
fi

AC_ARG_WITH(memacs,
AC_HELP_STRING([--without-memacs],[leave out everything that depends on
                         terminal cursor capabilities])
)

if test "$with_memacs" = "yes" ; then
dnl begining of with_memacs
NRN_CHECK_LIB_TERMCAP
AC_SUBST(TERMCAP_LIB)

dnl Check for the readline library.
if test "$with_readline" = ""; then
  if test "$CYGWIN" = "yes" ; then
    with_readline=no #use ours. we hacked so neuron exits when rxvt closed.
  else
    with_readline=yes		# Readline should be there.
  fi
fi
AC_ARG_WITH(readline,
AC_HELP_STRING([-with-readline=dir],[Specify the location of the readline library.  You
                          must specify this option if readline is not located
                          in a standard place.  The configure script looks
			  for the library in the subdirectory lib underneath
			  the directory you specify.])
,
[if test "$with_readline" = "yes"; then # default
   READLINE_LIBS="-lreadline $TERMCAP_LIB"
 elif test "$with_readline" = "no" ; then # build our own
   echo "build our own version of libreadline"
   READLINE_LIBS="-lreadline $TERMCAP_LIB"
   build_readline=yes
   NRN_READLINE_LIBS="../readline/libreadline.la $TERMCAP_LIB"
   NRN_READLINE_DEP="../readline/libreadline.la"
 else # Location of readline specified?
   READLINE_LIBS="-L$with_readline/lib -lreadline $TERMCAP_LIB"
 fi

xLIBS="$LIBS"
if test "$build_readline" != "yes" ; then
 AC_MSG_CHECKING([checking compilation with -lreadline])
 LIBS="$LIBS $READLINE_LIBS"
 build_readline=no
   NRN_READLINE_LIBS="$READLINE_LIBS"
   NRN_READLINE_DEP=""
 AC_TRY_LINK([extern char * rl_deprep_terminal();], [rl_deprep_terminal()],
   [AC_MSG_RESULT(ok)],
   [cat << EOF
failed
Link with the readline library failed.
Will attempt to compile and link with the older version of readline
that comes with this installation. If this is not what you want, i.e.
you want to use the latest gnu version of readline and it
is not installed in a standard
place (e.g., /usr/lib or /usr/local/lib), you must specify the proper location
of libreadline using

  configure --with-readline=/where/you/put/it

The file libreadline.a (or libreadline.so or whatever) should be located in
/where/you/put/it/lib.

If you don't have readline installed, you can get it from the GNU web site,
www.gnu.org.
EOF
   build_readline="yes"
   NRN_READLINE_LIBS="../readline/libreadline.la $TERMCAP_LIB"
   NRN_READLINE_DEP="../readline/libreadline.la"
])
fi
LIBS="$xLIBS"       # Put the library flags back.
],[cat << EOF

The GNU readline library must be installed somewhere on your system before you
can compile neuron.  If you don't have it anywhere, you can get it from the
GNU web site, www.gnu.org.  If you install it in a non-standard location, you
must specify its location to this configure procedure using

  configure --with-readline=/where/you/put/it/lib

EOF
exit 1
])
MEMACSLIB="-lmemacs"
MEMACSLIBLA="../memacs/libmemacs.la"

else dnl end of with_memacs and beginning of without_memacs
echo "no memacs or anything else that depends on terminal capabilities"
READLINE_LIBS=""
NRN_READLINE_LIBS=""
NRN_READLINE_DEP=""
TERMCAP_LIB=""
AC_SUBST(TERMCAP_LIB)
AC_DEFINE(WITHOUT_MEMACS,1,[define if no terminal capabilities])
MEMACSLIB=""
MEMACSLIBLA=""
fi

AC_SUBST(READLINE_LIBS)
AC_SUBST(NRN_READLINE_LIBS)
AC_SUBST(NRN_READLINE_DEP)
AM_CONDITIONAL(BUILD_MEMACS, test x$with_memacs = xyes)
AC_SUBST(MEMACSLIB)
AC_SUBST(MEMACSLIBLA)
])dnl end of AC_NRN_READLINE
