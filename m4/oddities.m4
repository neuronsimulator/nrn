dnl special problem work arounds
AC_DEFUN([AC_NRN_ODDITIES],[

dnl bison exists but it doesnt work
if test -z "$mlh_yacc" ; then
	case "$host" in
	alpha-cray-unicosmk* )
echo "Setting YACC=yacc since bison once didn't work on this machine at SDSC"
		YACC=yacc
	;;
	esac
fi

dnl parse or lex does not compile
case "$host" in
alpha-cray-unicosmk* )
	echo "Touching *.y and *.l since on this machine the distributed"
	echo "c files may not compile."
	touch `find . -name \*.\[[yl\]] -print`
;;
esac

dnl g++-2.8.1 compiler internal errors
if test -z "$mlh_cxxflag" ; then
	if test "$GXX"='yes' ; then
		zzz="`$CXX -v 2>&1 | sed -n '2s/ /_/gp'`"
		case "$zzz" in
		*version_2.8.1 ) dnl -O fails on deck2_6.cpp and xfont.cpp
echo "Compiling several c++ files will fail with an internal error"
echo "because of a g++-2.8.1 optimization bug. Setting CXXFLAGS=-g"
			CXXFLAGS=-g
		;;
		esac
	fi
fi

dnl Too many ld warnings
if test -z "$mlh_ldflag" ; then
	case "$host_os" in
	irix6* )
	echo "This machine gives so many ld warnings that it fails. So:"
	echo "setting LDFLAGS=-w"
	LDFLAGS="$LDFLAGS -w"
	;;
	darwin6* )
	LDFLAGS="$LDFLAGS -Wl,-bind_at_load"
	echo "This machine requires LDFLAGS=$LDFLAGS"
	;;
	esac
fi

dnl Does a signal call need a cast for the handler.
AC_LANG_PUSH([C++])
AC_TRY_LINK([
#include <signal.h>
RETSIGTYPE sighand(int) {}
],[
	signal(SIGUSR1, sighand);
	return 0;
],[
echo "The signal function does not need a cast for the handler"
],[
AC_TRY_LINK([
#include <signal.h>
RETSIGTYPE sighand(int) {}
],[
	signal(SIGUSR1, (RETSIGTYPE(*)(...))sighand);
	return 0;
],[
AC_DEFINE(SIGNAL_CAST,RETSIGTYPE(*)(...),[define if RETSIGTYPE(*)(int) is not the prototype for a signal handler])
echo "The signal function needs a cast to RETSIGTYPE(*)(...) for the handler"
],[
echo "The signal function needs an unknown cast for the handler"
echo "Neither RETSIGTYPE (*)(int) or RETSIGTYPE (*)(...) are correct prototypes."
])
])
AC_LANG_POP([])

dnl see src/mac/after_install . Allow safe changing of install_name
case "$host_os" in
  darwin*)
	LDFLAGS="$LDFLAGS -headerpad_max_install_names"
	;;
esac

])dnl end of AC_NRN_ODDITIES
