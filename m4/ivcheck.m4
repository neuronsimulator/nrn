dnl There are several options with regard to how much InterViews is
dnl actually used
dnl --without-nrniv means do not use it at all. ie build only nrnoc
dnl --without-iv means to build as much of nrniv as possible using only
dnl	the OS part of interviews which becomes built-in to ivoc and
dnl	is found in src/ivos

AC_DEFUN([AC_NRN_InterViews],[

AC_LANG_PUSH([C++])
AC_CXX_HAVE_SSTREAM

dnl Check for interviews.  If --with-iv=dir is specified, then 
if test "$with_iv" = ""; then
  with_iv=yes			# Enable interviews by default.
fi

if test "$with_nrniv" = ""; then
  with_nrniv=yes			# Enable nrniv by default
fi
use_ivos=no
build_nrniv=yes

AC_ARG_WITH(x,
AC_HELP_STRING([--without-x],[disable everything depending on x11
equivalent to --without-iv --without-nrnoc-x11 --without-x
]),[
	if test "$with_x" = "no"; then
		echo "disable everything depending on x11"
		  use_ivos=yes
		  with_iv=no
		  with_nrnoc_x11=no
	fi
])

AC_ARG_WITH(nrniv,
AC_HELP_STRING([--with-nrniv],[This is the default.
                             Build as much of nrniv as is consistent
                             with the existence or non-existence
                             of InterViews. I.e if InterViews is not found
                             List, Vector, File, Impedance, SaveState, etc.
                             will still be built.])

AC_HELP_STRING([--without-nrniv],[Build only nrnoc])
,[
	if test "$with_nrniv" = "no" ; then
		dnl take away with the left what has been given with the right
		echo "Not building nrniv and"
		echo "not building in any subdirectories that are only"
		echo "needed by nrniv"
		HAVE_IV=0
		IV_LIBS=""
		IV_DIR=""
		IV_INCLUDE=""
		IV_LIBS_LIBTOOL=""
		build_nrniv=no
	fi
])

if test "$build_nrniv" = "yes" ; then

AC_ARG_WITH(iv,
AC_HELP_STRING([--with-iv=dir],[Specify the location of the InterViews graphics
                          package.  You must specify this option if you
                          do not have them installed in a standard place.
			  The standard places are (from high to low precedence)
                            prefix/../iv  /usr/local/iv  /usr/local])
AC_HELP_STRING([--without-iv],[Do not compile graphics into neuron.])
,[
	dnl Interviews was specified:
	HAVE_IV=1
	IVHINES=IVhines
	if test "$with_iv" = "no" ; then
		dnl Interviews is not desired:
		HAVE_IV=0
		IVHINES=ivos
		echo "Not compiling with interviews."
	elif test "$with_iv" = "yes" ; then # look in default places
		echo "Looking for InterViews in the usual places--"
		dnl calculate prefix of prefix
		xprefix=`echo "$prefix" | sed 's;/[[^/]]*$;;'`
		dnl check in prefix/../iv
		if test -d $xprefix/iv ; then
			IV_DIR=$xprefix/iv
			IV_LIBDIR="$IV_DIR"/"$host_cpu"/lib
			if test ! -d $IV_LIBDIR ; then
				IV_LIBDIR="$IV_DIR"/lib
			fi
			IV_LIBS="-L$IV_LIBDIR -lIVhines $X_LIBS"
			IV_LIBS_LIBTOOL="$IV_LIBDIR/libIVhines.la"
			IV_INCLUDE=-I$IV_DIR/include
		dnl check in /usr/local/iv
		elif test -d /usr/local/iv ; then
			echo " not in  $IV_DIR"
			IV_DIR=/usr/local/iv
			IV_LIBDIR="$IV_DIR"/"$host_cpu"/lib
			if test ! -d $IV_LIBDIR ; then
				IV_LIBDIR="$IV_DIR"/lib
			fi
			IV_LIBS="-L$IV_LIBDIR -lIVhines $X_LIBS"
			IV_LIBS_LIBTOOL="$IV_LIBDIR/libIVhines.la"
			IV_INCLUDE=-I$IV_DIR/include
		dnl a standard? place
		else
			echo " not in  $IV_DIR"
			IV_LIBS="-lIVhines $X_LIBS"
			IV_LIBS_LIBTOOL="$IV_LIBS"
		fi
		if test -n "$IV_DIR" ; then
			echo "Assuming top level iv directory at $IV_DIR"
			echo " and libraries in $IV_LIBDIR"
		else
			echo "Assuming InterViews in a standard? place."
		fi
	else  # Location specified
	  IV_DIR=$with_iv
	  IV_LIBDIR="$IV_DIR"/"$host_cpu"/lib
	  if test ! -d $IV_LIBDIR ; then
		  IV_LIBDIR="$IV_DIR"/lib
	  fi
	  IV_LIBS="-L$IV_LIBDIR -lIVhines $X_LIBS"
	  IV_LIBS_LIBTOOL="$IV_LIBDIR/libIVhines.la"
	  IV_INCLUDE=-I$IV_DIR/include
	fi	  

	if test "$enable_carbon" = "yes" ; then
		IV_LIBS="$IV_LIBS -framework Carbon"
		IV_LIBS_LIBTOOL="$IV_LIBS_LIBTOOL -framework Carbon"
	fi

	if test "$HAVE_IV" = 1 ; then
        dnl Make sure that compilation and linking works:
	AC_MSG_CHECKING([compilation with interviews])
	
	xLIBS=$LIBS
	LIBS="$IV_LIBS $LIBS"
	xCXXFLAGS=$CXXFLAGS
	CXXFLAGS="$CXXFLAGS $IV_INCLUDE"
	AC_TRY_LINK(
	 [#include "OS/string.h"], dnl Random include file.
	 [String xyz;], dnl Random function to make sure link works.
	 [AC_MSG_RESULT(ok)],
	 [cat << EOF
failed
I can't compile and/or link an interviews program.  If InterViews is not
installed in /usr or /usr/local or /usr/local/iv,
specify the proper location of the interviews tree using

	configure --with-iv=/where/you/put/iv

The interviews headers should be in subdirectories of
/where/you/put/iv/include, and the libraries should be in
/where/you/put/iv/lib or /where/you/put/iv/${host_cpu}/lib
 (not in /where/you/put/iv/lib/ALPHA or
 /where/you/put/iv/lib/LINUX or some other subdirectory of lib).

If you don't want interviews, but do want nrniv built with the non-gui
c++ classes then specify configure --without-iv.
EOF
exit 1
])
	dnl Make sure we are up to date for nrn7.3
	AC_MSG_CHECKING([if InterViews is up to date with respect to nrn-7.4 and later])
	AC_TRY_COMPILE(
	 [#include <ivversion.h>],
	 [
#if (iv_hines_version < 19)
#error InterViews version is iv_hines_version
#endif
	 ],
	 [AC_MSG_RESULT(ok)],
	 [cat << EOF
failed
The installed version of InterViews is out of date. Install iv-19.tar.gz
EOF
exit 1
	 ])
	CXXFLAGS=$xCXXFLAGS
	LIBS=$xLIBS
	fi
])
fi

if test "$build_nrniv" = "yes" ; then
if test "$HAVE_IV" = 0 ; then
	dnl At least build the c++ stuff that does not
	dnl depend on anything more than src/ivos
echo "Building nrniv only with classes that do not depend on InterViews."
	use_ivos=yes
	IV_DIR=""
	IV_LIBS="$X_LIBS"
	IV_LIBS_LIBTOOL="$X_LIBS"
	IV_INCLUDE='-I$(top_srcdir)/src/ivos'
	IVOS_DIR="../ivos/libivos.la"
	IVOS_LIB="-livos"
fi
fi

AC_DEFINE_UNQUOTED(HAVE_IV, $HAVE_IV, define if using InterViews)
AC_SUBST(IV_LIBS)
AC_SUBST(IV_LIBS_LIBTOOL)
AC_SUBST(IV_INCLUDE)
AC_SUBST(IVOS_DIR)
AC_SUBST(IVOS_LIB)
AC_SUBST(IV_LIBDIR)
AC_SUBST(IVHINES)

AC_LANG_POP([])
])dnl end of AC_NRN_InterViews
