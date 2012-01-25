dnl I cant figure out how to hide AC_DEFINE so that autoheader does not
dnl put it in nrnconf.h.in (it will exist in some other header precursor.)
dnl 
AC_DEFUN([NRN_DEFINE],
[
AC_DEFINE($1,$2,[(Remove from nrnconf.h.in)])
])

AC_DEFUN([NRN_DEFINE_UNQUOTED],
[
AC_DEFINE_UNQUOTED($1,$2,[(Remove from nrnconf.h.in)])
])

dnl Check for signal macros
AC_DEFUN([NRN_CHECK_SIGNAL],
[
  AC_MSG_CHECKING([if $1 defined in signal.h])
  AC_TRY_COMPILE([#include <signal.h>
  ],[
	signal($1, SIG_DFL);
  ],[
	AC_DEFINE(HAVE_$1,1,[(Define if this signal exists)])
	AC_MSG_RESULT(yes)
  ],[
	AC_MSG_RESULT(no)
  ]
  )
])

dnl
dnl This was stolen from readlin-4.0. Hines changed the name from
dnl BASH_CHECK_LIB_TERMCAP
dnl Hines reversed the order from termcap, curses, ncurses in hopes that
dnl all linux boxes have ncurses and the rpm will not need any other installation
dnl This obviates prefer_curses. Also the gnutermcap makes no sense in
dnl our context and I replaced that last resort with an error
dnl

AC_DEFUN([NRN_CHECK_LIB_TERMCAP],
[
if test "X$nrn_cv_termcap_lib" = "X"; then
_nrn_needmsg=yes
else
AC_MSG_CHECKING(which library has the termcap functions)
_nrn_needmsg=
fi
AC_CACHE_VAL(nrn_cv_termcap_lib,
[AC_CHECK_LIB(ncurses, tgetent, nrn_cv_termcap_lib=libncurses,
    [AC_CHECK_LIB(curses, tgetent, nrn_cv_termcap_lib=libcurses,
	[AC_CHECK_LIB(termcap, tgetent, nrn_cv_termcap_lib=libtermcap,
	    [AC_MSG_ERROR([cannot find one of ncurses, curses, or termcap])])])])])
if test "X$_nrn_needmsg" = "Xyes"; then
AC_MSG_CHECKING(which library has the termcap functions)
fi
AC_MSG_RESULT(using $nrn_cv_termcap_lib)
if test $nrn_cv_termcap_lib = libtermcap ; then
TERMCAP_LIB=-ltermcap
TERMCAP_DEP=
elif test $nrn_cv_termcap_lib = libncurses ; then
TERMCAP_LIB=-lncurses
TERMCAP_DEP=
else
TERMCAP_LIB=-lcurses
TERMCAP_DEP=
fi
])

dnl Stolen from the autoconf archive
dnl @synopsis AC_CXX_NAMESPACES
dnl
dnl If the compiler can prevent names clashes using namespaces, define
dnl HAVE_NAMESPACES.
dnl
dnl @version $Id: acinclude.m4 1883 2007-11-10 19:32:20Z hines $
dnl @author Luc Maisonobe
dnl
AC_DEFUN([AC_CXX_NAMESPACES],
[AC_CACHE_CHECK(whether the compiler implements namespaces,
ac_cv_cxx_namespaces,
[AC_LANG_PUSH([C++])
 AC_TRY_COMPILE([namespace Outer { namespace Inner { int i = 0; }}],
                [using namespace Outer::Inner; return i;],
 ac_cv_cxx_namespaces=yes, ac_cv_cxx_namespaces=no)
 AC_LANG_POP([])
])
if test "$ac_cv_cxx_namespaces" = yes; then
  AC_DEFINE(HAVE_NAMESPACES,,[define if the compiler implements namespaces])
fi
])


dnl @synopsis AC_CXX_HAVE_STL
dnl
dnl If the compiler supports the Standard Template Library, define HAVE_STL.
dnl
dnl @version $Id: acinclude.m4 1883 2007-11-10 19:32:20Z hines $
dnl @author Luc Maisonobe
dnl
AC_DEFUN([AC_CXX_HAVE_STL],
[AC_CACHE_CHECK(whether the compiler supports Standard Template Library,
ac_cv_cxx_have_stl,
[AC_REQUIRE([AC_CXX_NAMESPACES])
 AC_LANG_PUSH([C++])
 AC_TRY_COMPILE([#include <list>
#include <deque>
#ifdef HAVE_NAMESPACES
using namespace std;
#endif],[list<int> x; x.push_back(5);
list<int>::iterator iter = x.begin(); if (iter != x.end()) ++iter; return 0;],
 ac_cv_cxx_have_stl=yes, ac_cv_cxx_have_stl=no)
 AC_LANG_POP([])
])
if test "$ac_cv_cxx_have_stl" = yes; then
  NRN_DEFINE(HAVE_STL,,[define if the compiler supports Standard Template Library])
fi
])

dnl Available from the GNU Autoconf Macro Archive at:
dnl http://www.gnu.org/software/ac-archive/htmldoc/ac_jni_include_dirs.html
dnl
AC_DEFUN([AC_JNI_INCLUDE_DIR],[

JNI_INCLUDE_DIRS=""

test "x$JAVAC" = x && AC_MSG_ERROR(['$JAVAC' undefined])
AC_PATH_PROG(_ACJNI_JAVAC, $JAVAC, no)
test "x$_ACJNI_JAVAC" = xno && AC_MSG_ERROR([$JAVAC could not be found in path])

_ACJNI_FOLLOW_SYMLINKS("$_ACJNI_JAVAC")
_JTOPDIR=`echo "$_ACJNI_FOLLOWED" | sed -e 's://*:/:g' -e 's:/[[^/]]*$::'`
case "$host_os" in
        darwin*)        _JTOPDIR=`echo "$_JTOPDIR" | sed -e 's:/[[^/]]*$::'`
                        _JINC="$_JTOPDIR/Headers";;
        *)              _JINC="$_JTOPDIR/include";;
esac
if test -f "$_JINC/jni.h"; then
        JNI_INCLUDE_DIRS="$JNI_INCLUDE_DIRS $_JINC"
else
        _JTOPDIR=`echo "$_JTOPDIR" | sed -e 's:/[[^/]]*$::'`
        if test -f "$_JTOPDIR/include/jni.h"; then
                JNI_INCLUDE_DIRS="$JNI_INCLUDE_DIRS $_JTOPDIR/include"
        else
                AC_MSG_ERROR([cannot find java include files])
        fi
fi

# get the likely subdirectories for system specific java includes
# Hines added cygwin. The example was for /cygdrive/c/j2sdk1.4.1_02
case "$host_os" in
bsdi*)          _JNI_INC_SUBDIRS="bsdos";;
linux*)         _JNI_INC_SUBDIRS="linux genunix";;
osf*)           _JNI_INC_SUBDIRS="alpha";;
solaris*)       _JNI_INC_SUBDIRS="solaris";;
cygwin*)	_JNI_INC_SUBDIRS="win32";;
*)              _JNI_INC_SUBDIRS="genunix";;
esac

# add any subdirectories that are present
for JINCSUBDIR in $_JNI_INC_SUBDIRS
do
        if test -d "$_JTOPDIR/include/$JINCSUBDIR"; then
                JNI_INCLUDE_DIRS="$JNI_INCLUDE_DIRS $_JTOPDIR/include/$JINCSUBDIR"
        fi
done
])

# _ACJNI_FOLLOW_SYMLINKS <path>
# Follows symbolic links on <path>,
# finally setting variable _ACJNI_FOLLOWED
# --------------------
AC_DEFUN([_ACJNI_FOLLOW_SYMLINKS],[
# find the include directory relative to the javac executable
_cur="$1"
while ls -ld "$_cur" 2>/dev/null | grep " -> " >/dev/null; do
        AC_MSG_CHECKING(symlink for $_cur)
        _slink=`ls -ld "$_cur" | sed 's/.* -> //'`
        case "$_slink" in
        /*) _cur="$_slink";;
        # 'X' avoids triggering unwanted echo options.
        *) _cur=`echo "X$_cur" | sed -e 's/^X//' -e 's:[[^/]]*$::'`"$_slink";;
        esac
        AC_MSG_RESULT($_cur)
done
_ACJNI_FOLLOWED="$_cur"
])# _ACJNI


dnl Available from the GNU Autoconf Macro Archive at:
dnl http://www.gnu.org/software/ac-archive/htmldoc/am_rpm_init.html
dnl
dnl AM_RPM_INIT
dnl Figure out how to create rpms for this system and setup for an
dnl automake target

AC_DEFUN([AM_RPM_INIT],
[dnl
AC_REQUIRE([AC_CANONICAL_HOST])
dnl Find the RPM program
AC_ARG_WITH(rpm-prog,[  --with-rpm-prog=PROG   Which rpm to use (optional)],
            rpm_prog="$withval", rpm_prog="")

AC_ARG_ENABLE(rpm-rules, [  --enable-rpm-rules       Try to create rpm make rules (defaults to yes for Linux)],
                enable_rpm_rules="$withval",enable_rpm_rules=no)

AC_ARG_WITH(rpm-extra-args, [  --with-rpm-extra-args=ARGS       Run rpm with extra arguments (defaults to none)],
                rpm_extra_args="$withval", rpm_extra_args="")

dnl AC_ARG_ENABLE(rpm-topdir, [  --enable-rpm       Try to create rpm make rules (defaults to yes for Linux)],
dnl             enable_rpm_rules="$withval",no)

dnl echo enable_rpm_rules is $enable_rpm_rules
dnl echo rpm_prog is $rpm_prog

  RPM_TARGET=""

  if test x$enable_rpm_rules = xno ; then
     echo "Not trying to build rpms for your system (use --enable-rpm-rules to override) "
     no_rpm=yes
  else
    if test x$rpm_prog != x ; then
       if test x${RPM_PROG+set} != xset ; then
          RPM_PROG=$rpm_prog
       fi
    fi


    AC_PATH_PROG(RPM_PROG, rpmbuild, no)
    if test "$RPM_PROG" = "no" ; then
        AC_PATH_PROG(RPM_PROG, rpm, no)
    fi
    no_rpm=no
    if test "$RPM_PROG" = "no" ; then
echo *** RPM Configuration Failed
echo *** Failed to find the rpm program.  If you want to build rpm packages
echo *** indicate the path to the rpm program using  --with-rpm-prog=PROG
      no_rpm=yes
      RPM_MAKE_RULES=""
    else
      AC_MSG_CHECKING(how rpm sets %{_rpmdir})
      rpmdir=`rpm --eval %{_rpmdir}`
      if test x$rpmdir = x"%{_rpmdir}" ; then
        AC_MSG_RESULT([not set (cannot build rpms?)])
        echo *** Could not determine the value of %{_rpmdir}
        echo *** This could be because it is not set, or your version of rpm does not set it
        echo *** It must be set in order to generate the correct rpm generation commands
        echo ***
        echo *** You might still be able to create rpms, but I could not automate it for you
        echo *** BTW, if you know this is wrong, please help to improve the rpm.m4 module
        echo *** Send corrections, updates and fixes to dhawkins@cdrgts.com.  Thanks.
      else
        AC_MSG_RESULT([$rpmdir])
      fi
      AC_MSG_CHECKING(how rpm sets %{_rpmfilename})
      rpmfilename=$rpmdir/`rpm --eval %{_rpmfilename} | sed "s/%{ARCH}/${host_cpu}/g" | sed "s/%{NAME}/$PACKAGE/g" | sed "s/%{VERSION}/${VERSION}/g" | sed "s/%{RELEASE}/${RPM_RELEASE}/g"`
      AC_MSG_RESULT([$rpmfilename])

      RPM_DIR=${rpmdir}
      RPM_TARGET=$rpmfilename
      RPM_ARGS="-ta $rpm_extra_args"
      RPM_TARBALL=${PACKAGE}-${VERSION}.tar.gz
    fi
  fi

  case "${no_rpm}" in
    yes) make_rpms=false;;
    no) make_rpms=true;;
    *) AC_MSG_WARN([bad value ${no_rpm} for no_rpm (not making rpms)])
       make_rpms=false;;
  esac
  AC_SUBST(RPM_DIR)
  AC_SUBST(RPM_TARGET)
  AC_SUBST(RPM_ARGS)
  AC_SUBST(RPM_TARBALL)

  RPM_CONFIGURE_ARGS=${ac_configure_args}
  AC_SUBST(RPM_CONFIGURE_ARGS)
])

dnl If the C++ library has a working stringstream, define HAVE_SSTREAM
dnl Ben Stanley 
dnl 1.1 (2001/03/16)

AC_DEFUN([AC_CXX_HAVE_SSTREAM],
[AC_CACHE_CHECK(whether the compiler has stringstream,
ac_cv_cxx_have_sstream,
[AC_REQUIRE([AC_CXX_NAMESPACES])
 AC_LANG_PUSH([C++])
 AC_TRY_COMPILE([#include <sstream>
#ifdef HAVE_NAMESPACES
using namespace std;
#endif],[stringstream message; message << "Hello"; return 0;],
 ac_cv_cxx_have_sstream=yes, ac_cv_cxx_have_sstream=no)
 AC_LANG_POP([])
])
if test "$ac_cv_cxx_have_sstream" = yes; then
  AC_DEFINE(HAVE_SSTREAM,,[define if the compiler has stringstream])
fi
])

dnl decide whether to use std::fabs or ::fabs or declare it explicitly
AC_DEFUN([NRN_FABS],[
 AC_LANG_SAVE
 AC_LANG_CPLUSPLUS
 AC_TRY_COMPILE([
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#define myfabs ::fabs
 ],[
	double d;
	d = -25.0;
	d = myfabs(d);
	return (d == 25.0)?0:1;
 ],
	ivos_fabs="::fabs" , ivos_fabs=""
 )
 if test "$ivos_fabs" = "" ; then
   AC_TRY_COMPILE([
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#define myfabs std::fabs
   ],[
	double d;
	d = -25.0;
	d = myfabs(d);
	return (d == 25.0)?0:1;
   ],
	ivos_fabs="std::fabs" , ivos_fabs=""
   )
 fi
 if test "$ivos_fabs" != "" ; then
	AC_DEFINE_UNQUOTED(IVOS_FABS,$ivos_fabs,[undefined or ::fabs or std::fabs])
 fi

 AC_TRY_COMPILE([
#include <stdlib.h>
#include <stddef.h>
#include <math.h>
float abs(float arg);
inline float abs(float arg)
{
  return (arg < 0.0)? -arg : arg;
}
 ],[
	return 0;
 ],
	AC_DEFINE(INLINE_FLOAT_ABS,1,[define if can declare inline float abs(float)])
 )

 AC_TRY_COMPILE([
#include <stdlib.h>
#include <stddef.h>
#include <math.h>
long abs(long arg);
inline long abs(long arg)
{
  return (arg < 0.0)? -arg : arg;
}
 ],[
	return 0;
 ],
	AC_DEFINE(INLINE_LONG_ABS,1,[define if can declare inline long abs(long)])
 )

 AC_LANG_RESTORE
])dnl


