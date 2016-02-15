dnl Process this file with autoconf to produce a configure script.

dnl disable caching. When configure fails, it is best to start from scratch.
define([AC_CACHE_LOAD], )dnl
define([AC_CACHE_SAVE], )dnl

dnl when you change following also update svn2nrn_h.sh and hg2nrnversion_h.sh
AC_INIT([nrn],[7.4])
PACKAGE_VERSION_MAJOR=`echo "$PACKAGE_VERSION" | sed 's/\..*//'`
PACKAGE_VERSION_MINOR=`echo "$PACKAGE_VERSION" | sed 's/^.*\.//'`
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_SRCDIR(src/memacs/main.c)
AM_INIT_AUTOMAKE

dnl The following line causes us to make a config.h instead of putting all
dnl the defines into the makefile.
AC_CONFIG_HEADERS(config.h)
AH_TOP([
#ifndef H_nrnconf_included
#define H_nrnconf_included 1
])
AH_BOTTOM([
#if defined(__cplusplus)
#include <ivstream.h>
#endif

#if defined(CYGWIN) || defined(MINGW)
#define WIN32 1
#endif

#if defined(MINGW)
#define CYGWIN
#endif

#if defined(carbon) && defined(__cplusplus)
#define MAC 1
#include <Carbon/Carbon.h>
#endif

#endif /* H_nrnconf_included */
])

dnl following does not seem to work in the sense of
dnl causing problems with exec_prefix
AC_PREFIX_DEFAULT(/usr/local/nrn)
dnl so do it explicitly
if test "$prefix" = NONE ; then
        prefix=/usr/local/nrn
fi

if test "$prefix" = "NONE" ; then
prefix=$ac_default_prefix
fi

AC_CANONICAL_HOST  dnl Make generic macros for the kind of system.

dnl for later testing with regard to special work arounds
dnl if the user has taken the trouble to set them then we don't want
dnl to re-set them
mlh_cc="$CC"
mlh_cflag="$CFLAGS"
mlh_cxx="$CXX"
mlh_cxxflag="$CXXFLAGS"
mlh_yacc="$YACC"
mlh_ldflag="$LDFLAGS"

dnl dont want prefix/share/nrn to be the default data directory
dnl if test "$datadir" = '${prefix}/share' && test "$prefix" != '/usr/local' ; then
dnl	datadir='${prefix}'
dnl fi

AC_ARG_ENABLE([UniversalMacBinary],
	AC_HELP_STRING([--enable-UniversalMacBinary], [combined binary for ppc and i386, fake arch is called umac]),[
	enable_dependency_tracking=no
	CFLAGS="$CFLAGS -arch ppc -arch i386"
	CXXFLAGS="$CXXFLAGS -arch ppc -arch i386"
	host_cpu=umac
])
AM_CONDITIONAL(UniversalMacBinary, test x$enable_UniversalMacBinary = xyes)

AC_ARG_ENABLE([DiscreteEventObserver],
	AC_HELP_STRING([--disable-DiscreteEventObserver], [Save space, but danger of use of freed memory if any NetCon destroyed.]),[
	if test "$enable_DiscreteEventObserver" != "no" ; then
		enable_DiscreteEventObserver="yes"
	fi
],[
	enable_DiscreteEventObserver=yes
])
if test "$enable_DiscreteEventObserver" = "yes" ; then
	NRN_DEFINE(DISCRETE_EVENT_OBSERVER,1,[Undef if do not want Observer to be a subclass of DiscreteEvent])
fi

AC_ARG_WITH([nmodl-only],
	AC_HELP_STRING([--with-nmodl-only],[When cross compiling, nocmodl need to be compiled for the build machine]), [
	if test "$with_nmodl_only" = "yes" ; then
		nmodl_build=yes
	fi
],[
	with_nmodl_only=no
])

AC_ARG_WITH([nmodl], 
	AC_HELP_STRING([--without-nmodl],[When cross compiling, nocmodl should NOT be compiled for the host machine]), [
	if test "$with_nmodl" = "yes" ; then
		nmodl_build=yes
	else
		nmodl_build=no
	fi
],[
	nmodl_build=yes
])

AC_NRN_BLUEGENE

if test x$with_nmodl_only = xyes ; then
	nmodl_build=yes
fi
if test "$nmodl_build" != "no" ; then
	nmodl_build=yes
fi

AM_CONDITIONAL(NMODL_ONLY, test x$with_nmodl_only = xyes)
AM_CONDITIONAL(NMODL_BUILD, test x$nmodl_build = xyes)

if test "$always_call_mpi_init" = "yes" ; then
	NRN_DEFINE(ALWAYS_CALL_MPI_INIT,1,[define if needed])
fi
if test "$file_open_retry" != "" ; then
	NRN_DEFINE_UNQUOTED(FILE_OPEN_RETRY,$file_open_retry,[Number of times to retry a failed open])
fi

dnl Use to be only when configured with mpi. Now always since it is so useful.
NRN_DEFINE_UNQUOTED(DLL_DEFAULT_FNAME,"$host_cpu/.libs/libnrnmech.so",[Define if want a automatic dll load])

dnl there are some os dependent properties in nrn.defaults.
nrndef_unix="//"
nrndef_mac="//"
nrndef_mswin="//"

case $host_os in
	cygwin* ) CYGWIN=yes
		  MINGW=no;;
	mingw* )  MINGW=yes
		  CYGWIN=no;;
	* )	  CYGWIN=no
		  MINGW=no;;
esac

echo "CYGWIN=$CYGWIN"
echo "MINGW=$MINGW"
dnl for mswin, rebase will be skipped only for mingw64
nrnskip_rebase=''

if test "$CYGWIN" = "yes" -o "$MINGW" = "yes" ; then
	nrndef_mswin=""
fi

dnl for some unknown reason this is needed to tickle the use of mpic++
dnl if requested later.
AC_LANG_SAVE
AC_LANG_CPLUSPLUS
AC_TRY_COMPILE([],[return 0;],,)
AC_LANG_RESTORE

AM_CONDITIONAL(BUILD_CARBON, test x$enable_carbon = xyes)

NRN_SRCDIR=$srcdir
AC_SUBST(NRN_SRCDIR)
dnl use the --enable-rpm-rules=yes
RPM_RELEASE="`export NSRC=$srcdir ; sh $srcdir/nrnversion.sh commit`"
AC_SUBST(RPM_RELEASE)
AC_SUBST(PACKAGE_VERSION_MAJOR)
AC_SUBST(PACKAGE_VERSION_MINOR)
if test "$rpm_extra_args" = "" ; then
	rpm_extra_args="-ba nrnrpm.spec"
fi
AM_RPM_INIT
dnl Enable or disable the rpm making rules in Makefile.am
AM_CONDITIONAL(MAKE_RPMS, test x$make_rpms = xtrue)
dnl rpm is most generic when linked statically
dnl however we would need to deal with libnrnmech.so and dlopen
if test x$make_rpms = xtrue ; then
dnl	AM_ENABLE_SHARED(no)
	AM_ENABLE_SHARED(yes)
	if test "$with_readline" = "" ; then
		with_readline="no"
	fi
elif test x$CYGWIN = xyes ; then
dnl shared libraries cannot have undefined names. src/mswin/nrncygso.sh
dnl will combine .o files into a single nrniv.dll (-fPIC is not needed)
	AM_ENABLE_SHARED(no)
	AM_ENABLE_STATIC(yes)
elif test x$MINGW = xyes ; then
	AM_ENABLE_SHARED(no)
	AM_ENABLE_STATIC(yes)
else
	AM_ENABLE_SHARED(yes)
	AM_ENABLE_STATIC(no)
fi

dnl Checks for programs.
AC_PROG_CC
AC_PROG_CPP
AC_PROG_AWK
AC_PROG_INSTALL
dnl AM_PROG_LEX has a bug in it, but it's required by automake 1.4.  We
dnl replace this by a call to AC_PROG_LEX.  See acinclude.m4.  This should
dnl probably be undone for the next version of automake.
AM_PROG_LEX
AC_PROG_YACC
AC_PROG_LN_S
dnl following line allows java to be the controller for NEURON
AC_LIBTOOL_DLOPEN
dnl Older versions of automake insist on having the AM_PROG_LIBTOOL,
dnl although it is now obsolete.  We used to leave it in here, and then define
dnl AM_PROG_LIBTOOL to call AC_PROG_LIBTOOL, which is the modern
dnl way of doing it.

dnl Checks for libraries
AC_CHECK_LIBM
LIBS="$LIBS $LIBM"

dnl Python for cygwin and mingw has to be done early because later tests may
dnl have to be done with a CFLAGS="-mno-cygwin" (obsolete after gcc2.3)
dnl What to do about Python
AC_NRN_PYTHON

dnl Python that runs on the build machine needed to create hocusr.h
AC_CHECK_PROG(PYTHON_BLD, python, python)

dnl Only needed if making a distribution. In that case cython must work.
dnl Otherwise, to make a distribution, need to modify
dnl src/neuronmusic/Makefile.am and remove neuronmusic.cpp from the list
dnl of extra dist files.
dnl Also needed for neuron.rxd.geometry3d module
if test "x$have_cython" != "xno" ; then
  AC_CHECK_PROG(have_cython, cython, yes, no)
fi
AC_CHECK_PROG(CYTHON, cython, cython, cython)
AM_CONDITIONAL(HAVE_CYTHON, test x$have_cython = xyes)

BUILD_RX3D=0
if test "$ac_nrn_rx3d" = "yes" ; then
	if test x$have_cython = xyes -o -f share/lib/python/neuron/rxd/geometry3d/ctng.cpp ; then
		BUILD_RX3D=1
	else
		BUILD_RX3D=0
	fi
fi
AC_SUBST(BUILD_RX3D)
AM_CONDITIONAL(BUILD_RX3D, test "x$BUILD_RX3D" = "x1")

MINGW_CFLAG=""
EXTRA_CYGWIN_FLAGS='$(EXTRA_CYGWIN)'
if test "$CYGWIN" = "yes" ; then
	if test "$ac_nrn_cygwin" = no ; then
		AC_MSG_NOTICE([Build using MinGW.])
		MINGW_CFLAG="-mno-cygwin"
		EXTRA_CYGWIN_FLAGS=''
		dnl following will be put back at end
		CFLAGS="-mno-cygwin $CFLAGS"
		CXXFLAGS="-mno-cygwin $CXXFLAGS"
	fi
fi
AC_SUBST(MINGW_CFLAG)
AC_SUBST(EXTRA_CYGWIN_FLAGS)

AC_NRN_MUSIC
AC_NRN_PARANEURON
AC_NRN_WITH_MPI
AC_NRN_WITH_METIS

AM_PROG_LIBTOOL

dnl when purify is enabled then one can use
dnl nrnivmodl -p
dnl to create a special which linked with purify.
if test "$enable_purify" = "yes" ; then
	dnl special is a program, not a shell script for nrniv -dll...
	linux_nrnmech="no"
dnl I believe this is the way it should be done but then all the
dnl CXX variables would have to be set
dnl	xtagname=$tagname
dnl	tagname=purify
dnl	AC_LIBTOOL_CONFIG([purify])
dnl	tagname=$xtagname
dnl On the other hand, this appends to the end and seems to work.
dnl All we really wanted was to avoid the rpath args.
	cat << __EOF__ >> libtool
# ### BEGIN LIBTOOL TAG CONFIG: purify
hardcode_libdir_flag_spec=""
# ### END LIBTOOL TAG CONFIG: purify
__EOF__
fi
AM_CONDITIONAL(NRNPURIFY, test x$enable_purify = xyes)

dnl linux rpm works only with nrnmech.dll style
case "$host_os" in
  *linux*)
	if test x${linux_nrnmech} = x ; then
		linux_nrnmech="yes"
		addlibdl="yes"
	fi
	if test x${java_dlopen} = x ; then
		java_dlopen="yes"
		addlibdl="yes"
	fi
	;;
esac

case "$host_os" in
  darwin*)
	AC_DEFINE(DARWIN, 1, [if mac os x])
	macdarwin=yes
	if test x${linux_nrnmech} = x ; then
		linux_nrnmech="yes"
	fi
	if test x${java_dlopen} = x ; then
		java_dlopen="yes"
	fi
	nrndef_mac=""
	;;
esac

if test x${linux_nrnmech} = xyes ; then
	AC_DEFINE(NRNMECH_DLL_STYLE, 1,[if 1 then dlopen nrnmech instead of special])
	AC_MSG_NOTICE([Use the NRNMECH_DLL_STYLE for "special"])
else
	AC_MSG_NOTICE([Use the default unix style for "special" (not NRNMECH_DLL_STYLE)])
fi
AM_CONDITIONAL(NRNMECH_DLL_STYLE, test x${linux_nrnmech} = xyes)
AM_CONDITIONAL(MAC_DARWIN, test x${macdarwin} = xyes)

if test x${addlibdl} = xyes ; then
	LIBS="$LIBS -ldl"
fi

dnl what to do with the readline library and memacs
AC_NRN_READLINE

dnl What to do about nrnjava
AC_NRN_JAVA

dnl CYGWIN or X11. Former takes precedence
if test "$CYGWIN" = "yes" ; then
	no_x=yes
	echo " CYGWIN and no X11 so make MSWwin version"
	build_cygwin=yes
	AC_DEFINE(CYGWIN,,[define if using cygwin])
	if test "$marshal_dir" = "" ; then
		#do not change without changing instal.sh and nrnsetup.nsi
		marshal_dir=/cygdrive/c/marshalnrn
		binsuffix=""
	fi
	if  test "$LDFLAGS" = "" ; then
		LDFLAGS='-Wl,--enable-auto-import'
	fi
	AC_MSG_NOTICE(['make mswin' will marshal for building the installer in the marshal_dir = $marshal_dir])
	AC_SUBST(marshal_dir)
	AC_SUBST(binsuffix)
elif test "$MINGW" = "yes" ; then
	no_x=yes
	echo " MINGW and no X11 so make MSWwin version"
	build_mingw=yes
	AC_DEFINE(MINGW,,[define if using mingw])
	if test "$marshal_dir" = "" ; then
		#do not change without changing instalmingw.sh
		# and nrnsetupmingw.nsi
		if test "$host_cpu" = x86_64 ; then
			marshal_dir='c:/marshalnrn64'
			dos_marshal_dir='c:\marshalnrn64'
			binsuffix=""
			nrnskip_rebase='#'
		else
			marshal_dir='c:/marshalnrn'
			dos_marshal_dir='c:\marshalnrn'
			binsuffix=""
		fi
	fi
	AC_MSG_NOTICE(['make mswin' will marshal for building the installer in the marshal_dir = $marshal_dir])
	AC_SUBST(marshal_dir)
	AC_SUBST(dos_marshal_dir)
	AC_SUBST(binsuffix)
elif test "$enable_carbon" = "yes" ; then
	no_x=yes
	echo "Make the carbon version on max os x"
	AC_DEFINE(carbon,1,[define if using Carbon libraries])
else
	AC_PATH_XTRA   dnl Find X11.
	if test "$no_x" = "" ; then
		X_LIBS="$X_LIBS -lX11"
	fi
fi
AC_SUBST(enable_carbon)
AC_SUBST(nrnskip_rebase)

dnl Real time dynamic clamp
AC_NRN_REALTIME

dnl Contributions by collaborators
if test "$enable_MikeNeubig" = "yes" ; then
 AC_DEFINE(MikeNeubig,1,[define if using Mike Neubig <neubig@salk.edu> contributions])
fi

dnl Checks for header files.
AC_HEADER_DIRENT
AC_HEADER_STDC
AC_HEADER_SYS_WAIT
AC_CHECK_HEADERS(fcntl.h limits.h malloc.h sgtty.h strings.h sys/file.h)
AC_CHECK_HEADERS(sys/ioctl.h sys/time.h termio.h unistd.h stdarg.h varargs.h)
AC_CHECK_HEADERS(stropts.h sys/conf.h locale.h fenv.h pthread.h)

dnl but sometimes pthreads does not work so we use USE_PTHREAD
if test "$use_pthread" != "no" ; then
AC_TRY_COMPILE(
	[#include <pthread.h>],
	[
		pthread_cond_t cond;
		pthread_cond_init(&cond, (void*)0);
	],
	NRN_DEFINE(USE_PTHREAD,1,[Configure with use_pthead=no if you have pthread but do not want to use it])
		AC_MSG_NOTICE([Using pthreads.])
	,
	AC_MSG_NOTICE([Have pthread.h but not using threads.])
)
dnl here is a better way
ACX_PTHREAD([
	AC_MSG_NOTICE([successful.])
	echo "PTHREAD_LIBS=$PTHREAD_LIBS"
	echo "PTHREAD_CFLAGS=$PTHREAD_CFLAGS"
	echo "PTHREAD_CC=$PTHREAD_CC"
],[
	AC_MSG_NOTICE([did not succeed.])
])
fi

AC_CHECK_HEADERS(float.h math.h)
AC_DEFINE(SUNDIALS_DOUBLE_PRECISION, 1, [Define SUNDIALS data type 'realtype' as 'long double'])
AC_DEFINE(SUNDIALS_USE_GENERIC_MATH, 1, [Use generic math functions])

AC_LANG_PUSH([C++])
AC_CHECK_HEADERS(stream.h)
AC_LANG_POP([])

dnl Checks for typedefs, structures, and compiler characteristics.
AC_TYPE_OFF_T
AC_TYPE_PID_T
AC_TYPE_SIZE_T
m4_pattern_allow([^AC_TYPE_INT32_T$])
m4_pattern_allow([^AC_TYPE_UINT32_T$])
AC_TYPE_INT64_T

dnl This causes a warning message from automake and autoconf because, I
dnl suppose, it hasn't been updated.  There's nothing that can be done about
dnl this except to fix autoconf, since it comes from the autoconf distribution.
AC_C_BIGENDIAN

AC_HEADER_TIME
AC_STRUCT_TM

dnl Checks for library functions.
AC_FUNC_ALLOCA
AC_FUNC_MEMCMP
AC_TYPE_SIGNAL
AC_FUNC_VPRINTF
AC_CHECK_FUNCS(ftime getcwd getpw gethostname gettimeofday mkdir putenv setenv select strdup strstr index bzero bcopy stty lockf isatty mkstemp)
AC_CHECK_FUNCS(setitimer sigaction fesetround posix_memalign mallinfo)
NRN_CHECK_SIGNAL(SIGBUS)
NRN_CHECK_SIGNAL(SIGSEGV)
dnl Do this after the above checks, so they're run with the C compiler rather
dnl than the C++ compiler.  This is only a problem with complex.h, which
dnl always exists for the C++ compilers but meschach needs it for a C
dnl compiler.
AC_PROG_CXX
AC_CHECK_HEADERS(_G_config.h)

dnl std::fabs or ::fabs or explicitly declare
NRN_FABS

dnl Check for interviews.  If --with-iv=dir is specified, then 
AC_NRN_InterViews


if test "$nrndef_mac" = "//" -a "$nrndef_mswin" = "//" ; then
	nrndef_unix=""
fi

dnl put back the flags if mingw
if test "$CYGWIN" = "yes" ; then
	if test "$ac_nrn_cygwin" = no ; then
		CFLAGS="$mlh_CFLAGS"
		CXXFLAGS="$mlh_CXXFLAGS"
	fi
fi

case "$exec_prefix" in
	NONE*)  exec_prefix="${prefix}/${host_cpu}"
	;;
esac

dnl allow the nrnivmodl created subfolder to be explicitly specified
if test "$modsubdir" = "" ; then
	modsubdir=$host_cpu
fi
AC_SUBST(modsubdir)

AC_DEFINE_UNQUOTED(NRNHOSTCPU,"$host_cpu",[cpu type consistent with nrnivmodl])
NRN_LIBDIR=$exec_prefix/lib
AC_SUBST(NRN_LIBDIR)

AC_SUBST(nrndef_unix)
AC_SUBST(nrndef_mac)
AC_SUBST(nrndef_mswin)

AM_CONDITIONAL(BUILD_READLINE, test x$build_readline = xyes)
AM_CONDITIONAL(BUILD_NRNJAVA, test x$build_nrnjava = xyes)
AM_CONDITIONAL(BUILD_NRNIV, test x$build_nrniv = xyes)
AM_CONDITIONAL(BUILD_IVOS, test x$use_ivos = xyes)
AM_CONDITIONAL(BUILD_PARANEURON, test x$use_paranrn = xyes)
AM_CONDITIONAL(BUILD_NRNMPI_DYNAMIC, test x$nrnmpi_dynamic = xyes)
AM_CONDITIONAL(BUILD_CYGWIN, test x$build_cygwin = xyes)
AM_CONDITIONAL(BUILD_MINGW, test x$build_mingw = xyes)
AM_CONDITIONAL(BUILD_NRNPYTHON, test x$build_nrnpython = xyes)
AM_CONDITIONAL(BUILD_NRNPYTHON_DYNAMIC, test x$build_nrnpython_dynamic = xyes)

dnl special problem work arounds
AC_NRN_ODDITIES

dnl some configuration checking for ParallelContext
AC_NRN_ParallelContext

dnl This, in conjunction with a line in acconfig.h, causes the preprocessor
dnl symbol NEURON_DATA_DIR to point to the neuron data directory.
AC_DEFINE_UNQUOTED(NEURON_DATA_DIR, "$prefix/share/$PACKAGE", where the lib hoc is)
AC_DEFINE_UNQUOTED(NRNHOST, "$host", host triplet)

NRN_DEFINE_UNQUOTED(NRN_CONFIG_ARGS, "${ac_configure_args}", args given to configure)

NRNOC_X11=1
if test "$with_nrnoc_x11" = "" ; then
	with_nrnoc_x11=yes
fi

AC_ARG_WITH(nrnoc-x11,
AC_HELP_STRING([--with-nrnoc-x11],[This is the default
                                Add the x11 window code to NRNOC to allow
                                the old style x11 window for plot, axis, etc.])
AC_HELP_STRING([--without-nrnoc-x11],[Do not compile the old style nrnoc x11 window])
,[
	if test "$with_nrnoc_x11" = "no" ; then
		NRNOC_X11=0
	fi
])
if test "$no_x" = "yes" ; then
	NRNOC_X11=0
fi
echo "NRNOC_X11 = $NRNOC_X11"
AC_DEFINE_UNQUOTED(NRNOC_X11,$NRNOC_X11, if nrnoc can use X11)

dnl This list specifies what files configure actually makes.
AC_CONFIG_FILES([
	Makefile src/Makefile
])
if test "$nmodl_build" = "yes" ; then
AC_CONFIG_HEADERS([nmodlconf.h])
AC_CONFIG_FILES([
	src/modlunit/Makefile src/nmodl/Makefile
])
fi	
if test "$with_nmodl_only" != "yes" ; then
AC_CONFIG_HEADERS([nrnconf.h])
AC_CONFIG_HEADERS([src/nrnoc/nrnconfigargs.h])
AC_CONFIG_HEADERS([src/oc/nrnmpiuse.h])
AC_CONFIG_HEADERS([src/oc/nrnrtuse.h])
AC_CONFIG_HEADERS([src/oc/nrnpthread.h])
AC_CONFIG_HEADERS([src/parallel/bbsconf.h])
AC_CONFIG_HEADERS([src/nrnjava/njconf.h])
AC_CONFIG_HEADERS([src/nrncvode/nrnneosm.h])
AC_CONFIG_HEADERS([src/ivos/ivstream.h])
AC_CONFIG_HEADERS([src/sundials/sundials_config.h])
AC_CONFIG_HEADERS([src/nrnpython/nrnpython_config.h])

AC_CONFIG_FILES([
	nrnrpm.spec
	src/sundials/Makefile src/sundials/shared/Makefile
	src/sundials/cvodes/Makefile src/sundials/ida/Makefile
	src/sparse/Makefile src/memacs/Makefile
	src/readline/Makefile src/sparse13/Makefile src/Random123/Makefile
	src/oc/Makefile	src/scopmath/Makefile src/nrnoc/Makefile
	src/gnu/Makefile src/uxnrnbbs/Makefile src/mesch/Makefile
	src/uxnrnbbs/mos2nrn.h src/nrnmpi/Makefile
	src/ivoc/Makefile src/nrncvode/Makefile src/parallel/Makefile src/nrniv/Makefile
	src/ivos/Makefile src/ivos/InterViews/Makefile src/ivos/OS/Makefile
	src/nrnjava/Makefile src/nrnjava/nrnccmb/Makefile
	src/nrnjava/neuron/Makefile
	src/nrnpython/Makefile src/ni_pci_6229/Makefile
	src/mac/Makefile src/mswin/Makefile src/mswin/batch/Makefile
	src/mac/macdist.pkgproj src/mac/postinstall.sh
	src/mswin/nrnsetup.nsi src/mswin/nrncygso.sh src/mswin/lib/mknrndll.mak
	src/mswin/nrnsetupmingw.nsi
	src/mswin/EnvVarUpdate.nsh
	src/mswin/extra/Makefile src/mswin/rdln/Makefile
	src/mswin/windll/Makefile
	src/mswin/wnrnbbs/Makefile src/mswin/lib/Makefile
	src/mswin/bin/Makefile src/neuronmusic/Makefile src/neuronmusic/setup.py
	share/Makefile bin/nrnoc_makefile bin/nrniv_makefile bin/nrnmech_makefile
	share/lib/Makefile share/lib/hoc/Makefile share/lib/python/Makefile
	share/lib/hoc/celbild/Makefile share/lib/hoc/mulfit/Makefile
	share/lib/hoc/netbild/Makefile share/lib/hoc/lincir/Makefile
	share/lib/hoc/mview/Makefile share/lib/hoc/chanbild/Makefile
	share/lib/hoc/import3d/Makefile
	share/lib/auditscripts/Makefile
	share/lib/nrn.defaults
	bin/Makefile bin/nrnocmodl bin/nrnivmodl bin/neurondemo
	bin/nrngui bin/bbswork.sh
	bin/mos2nrn2.sh bin/hel2mos1.sh
	share/examples/Makefile share/examples/ivoc/Makefile
	share/examples/nrniv/Makefile share/examples/nrniv/cable/Makefile
	share/examples/nrniv/kkin/Makefile share/examples/nrniv/netcon/Makefile
	share/examples/nrniv/netsyn/Makefile share/examples/nrniv/nmodl/Makefile
	share/examples/nrniv/nrnoc/Makefile share/examples/nrniv/randomsyn/Makefile
	share/examples/nrniv/soma/Makefile share/examples/nrniv/vrecorder/Makefile
	share/examples/nrniv/xmech/Makefile
	share/demo/Makefile share/demo/release/Makefile share/demo/dend3/Makefile
	share/demo/sync/Makefile
	share/lib/python/neuron/rxd/geometry3d/Makefile
	share/lib/python/neuron/Makefile
	share/lib/python/neuron/rxd/Makefile
	])
AC_CONFIG_FILES([src/nrnpython/setup.py share/lib/python/neuron/rxd/geometry3d/setup.py],
  [if test $NRNPYTHON_PYMAJOR -gt 2 ; then $PY2TO3 -w src/nrnpython/setup.py ; $PY2TO3 share/lib/python/neuron/rxd/geometry3d/setup.py ; fi],
  [NRNPYTHON_PYMAJOR=$NRNPYTHON_PYMAJOR PY2TO3=$PY2TO3]
)
fi

AC_OUTPUT
