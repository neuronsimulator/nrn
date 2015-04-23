dnl distutils.sysconfig.get_python_version()
dnl distutils.sysconfig.get_python_inc()

AC_DEFUN([AC_NRN_PYCONF],[
	dnl determine configuration if able to  run python
	ac_nrn_pyconf_val=""
	ac_nrn_pyconf_val=`$4 -c "import distutils.sysconfig
print (distutils.sysconfig.$2)" | tr -d '\r'`
	if test $? != 0 ; then
		AC_MSG_ERROR([could not run python in order to determine a
configuration variable.])
	fi
	if test "$ac_nrn_pyconf_val" = "" -o "$ac_nrn_pyconf_val" = "None" ; then
		[$1]=[$3]
echo "[$2]  '$ac_nrn_pyconf_val' returning '$[$1]'"
	else
		[$1]=${ac_nrn_pyconf_val}
echo "[$2]  '$ac_nrn_pyconf_val'"
	fi
])

AC_DEFUN([AC_NRN_RUNPYTHON], [
	AC_MSG_CHECKING([if python include files and libraries work])
	zzzLD_LIBRARY_PATH=${LD_LIBRARY_PATH}
	LD_LIBRARY_PATH="${LD_LIBRARY_PATH}:${PYLIBDIR}"
	export LD_LIBRARY_PATH
	zzzCFLAGS="$CFLAGS"
	zzzLIBS="$LIBS"
	CFLAGS="$CFLAGS -I${PYINCDIR}"
	LIBS="${PYLIBLINK} $LIBS"
	AC_TRY_LINK([
#include <Python.h>
	],[
	Py_Initialize();
	Py_Finalize();
	return 0;
	],[
		AC_MSG_RESULT(yes)
	],[
		AC_MSG_ERROR(could not run a test that used the python library.
Examine config.log to see error details. Something wrong with
	PYLIB=$PYLIB
or
	PYLIBDIR=$PYLIBDIR
or	
	PYLIBLINK=$PYLIBLINK
or
	PYINCDIR=$PYINCDIR
)
	],[
		AC_MSG_ERROR(Cross compiling not allowed)
	])
	CFLAGS="$zzzCFLAGS"
	LIBS="$zzzLIBS"
	LD_LIBRARY_PATH="$zzzLD_LIBRARY_PATH"
	export LD_LIBRARY_PATH
])

AC_DEFUN([AC_NRN_PYTHON],[

	NRNPYTHON_LIBLA=""
	NRNPYTHON_LIBS=""
	NRNPYTHON_DEP=""
	NRNPYTHON_INCLUDES=""
	npy_NRNPYTHON_INCLUDES=""
	NRNPYTHON_PYLIBLINK=""
	NRNPYTHON_PYMAJOR=2
	PY2TO3="2to3"
	build_nrnpython=no
	build_nrnpython_dynamic=no
	npy_apiver=""

	AC_ARG_ENABLE([pysetup],
		AC_HELP_STRING([--enable-pysetup=[installoption]],
 [Execute 'python setup.py install installoption' as the last
installation step.
--disable-pysetup or an installoption of 'no' means do NOT execute
'python setup.py...'
The default installoption is '--home=<prefix>']
),
		[ac_pysetup="$enableval"], [ac_pysetup='--home=$(prefix)']
	)
	if test "$ac_pysetup" = "yes" ; then
		ac_pysetup='--home=$(prefix)'
	fi

	AC_ARG_WITH([nrnpython],
		AC_HELP_STRING([--with-nrnpython=[desired python binary or 'dynamic']],
			[Python interpreter can be used (default is NO)
Probably need to set PYLIBDIR to find libpython...
and PYINCDIR to find Python.h
]),
		[ac_nrn_python="$withval"], [ac_nrn_python=no]
	)
	AM_CONDITIONAL(NRN_PYTHON_ON, test x$ac_nrn_python != xno)
	nrn_temp_cflags="$CFLAGS"
	AC_ARG_ENABLE([cygwin],
		AC_HELP_STRING([--disable-cygwin],
			[build as MINGW program. Only for mswin.]),
		[ac_nrn_cygwin=$enableval], [ac_nrn_cygwin=yes]
	)

	AC_ARG_ENABLE([rx3d],
		AC_HELP_STRING([--disable-rx3d],
			[Do not compile the cython translated 3-d rxd features]),
		[ac_nrn_rx3d=$enableval], [ac_nrn_rx3d=yes]
	)

	if test "$ac_nrn_python" = "yes" ; then
		ac_nrn_python="python"
	fi

	if test "$ac_nrn_python" = "dynamic" ; then
		ac_nrn_python="python"
		build_nrnpython_dynamic="yes"
		NRN_DEFINE(USE_PYTHON,1,[Define if Python available])
		dnl 1013 good for 2.5-2.7, 1012 good for 2.3-2.4
		npy_apiver=`$ac_nrn_python -c "import sys;print sys.api_version,"`
		echo "dynamic npy_apiver=$npy_apiver"
		NRN_DEFINE_UNQUOTED(NRNPYTHON_DYNAMICLOAD,$npy_apiver,[Define to value of sys.api_version if dynamic loading desired])
	fi
	if test "$ac_nrn_python" != "no" ; then
		ac_nrn_python=`which ${ac_nrn_python}`

		if test "$ac_nrn_python" = "" ; then

			AC_MSG_ERROR([Either python is not in the path or the specified python does not exist.])

		fi

		echo "Python binary found ($ac_nrn_python)"
		PYTHON=$ac_nrn_python

		if test "$CYGWIN" = "yes" ; then
			dnl if python does not use cygwin then neither should we
			if test "$ac_nrn_cygwin" = "yes" ; then
cygcheck "$ac_nrn_python" | grep cygwin1.dll > /dev/null
				if test $? != 0 ; then
					ac_nrn_cygwin=no
					with_memacs=no
					with_readline=no
					with_iv=no
AC_MSG_NOTICE([Because this python is not a CYGWIN program, build as a MinGW program as though
 --disable-cygwin --with-readline=no --without-iv --without-memacs was invoked.
That is, build a version suitable mostly as a Python extension.])
				fi
			fi 
		fi
		AC_MSG_CHECKING([nrnpython configuration])
		NRN_DEFINE(USE_PYTHON,1,[define if Python available])
		if test "$PYVER" = "" ; then
			AC_NRN_PYCONF(xxx,get_python_version(),2.4,$ac_nrn_python)
			PYVER=${xxx}
			#Notice how brackets are escaped
			AC_NRN_PYCONF(xxx,sys.version_info@<:@0@:>@,2,$ac_nrn_python)
			#following only for 2.7 and 3.x
			#AC_NRN_PYCONF(xxx,sys.version_info.major,2,$ac_nrn_python)
			NRNPYTHON_PYMAJOR=${xxx}
		fi
		NRNPYTHON_PYVER="$PYVER"
		if test "$PYINCDIR" = "" ; then
			AC_NRN_PYCONF(xxx,get_python_inc(0),"",$ac_nrn_python)
			if test "$xxx" = "" ; then
AC_MSG_ERROR([cannot determine python include directory. Need to
explicitly specify PYINCDIR])
			else
				if test "$CYGWIN" = "yes" ; then xxx="`cygpath -u $xxx`" ; fi
				PYINCDIR="${xxx}"
			fi
		fi
		if test "$EXTRAPYLIBS" = "" ; then
			AC_NRN_PYCONF(EXTRAPYLIBS,get_config_var('LIBS'),"",$ac_nrn_python)
		fi
		setup_extra_link_args=extra_link_args
		case "$host_os" in
			darwin*)
				setup_extra_link_args='#extra_link_args'
				;;
		esac

		dnl standard hopefully
		if test "$PYLIB" = "" ; then
			AC_NRN_PYCONF(gcfLIBRARY, get_config_var('LIBRARY'),"",$ac_nrn_python)
			AC_NRN_PYCONF(gcfLIBDIR, get_config_var('LIBDIR'),"",$ac_nrn_python)
			AC_NRN_PYCONF(gcfLIBS, get_config_var('LIBS'),"",$ac_nrn_python)
			PYLIB=`echo $gcfLIBRARY|sed 's/lib\(.*\)\.a/\1/'`
			if test "$PYLIB" != "" ; then
				PYLIBDIR="$gcfLIBDIR"
				PYLIBLINK="-L$PYLIBDIR -l$PYLIB $gcfLIBS"
				PYLIB="$PYLIBLINK -R$PYLIBDIR"
			fi
		fi

		if test "$PYLIB" = "" ; then
			case "$host_os" in
			darwin*)
				AC_NRN_PYCONF(xxx,get_config_var('LINKFORSHARED'),"",$ac_nrn_python)
				PYLIBLINK="$xxx"
				PYLIB="$PYLIBLINK"
				;;
			*)
				AC_NRN_PYCONF(xxx,get_config_var('LINKFORSHARED'),"",$ac_nrn_python)
				PYLINKFORSHARED="$xxx"				
				if test "$host_os" = "cygwin" ; then
					AC_NRN_PYCONF(xxx,get_config_var('LIBPL'),"",$ac_nrn_python)
				else
					AC_NRN_PYCONF(xxx,get_config_var('LIBDIR'),"",$ac_nrn_python)
				fi
				if test "$xxx" == "" ; then
					xxx=1
					if test "$host_os" = "cygwin" -a "$ac_nrn_cygwin" = "no" ; then
PYLIBDIR="`dirname $ac_nrn_python`/libs"
if test -d "$PYLIBDIR" ; then
	PYLIB="`ls $PYLIBDIR/libpython*.a 2> /dev/null`"
	if  test "$PYLIB" != "" ; then
		PYLIB=`basename "$PYLIB" | sed 's/lib\(.*\)\.a/\1/'`
		PYLIBLINK="-L${PYLIBDIR} -l${PYLIB}"
		PYLIB="${PYLIBLINK}"
		xxx=0
	fi
fi
					fi
					if test "$xxx" = 1 ; then
AC_MSG_ERROR([Could not determine PYLIBDIR, explicitly set PYLIBDIR, PYLIB,
and PYLIBLINK.])
					fi
				else
PYLIBDIR="${xxx}"
PYLIBLINK="-L${PYLIBDIR} -lpython${PYVER} ${EXTRAPYLIBS}"
PYLIB="${PYLIBLINK} ${PYLINKFORSHARED} -R${PYLIBDIR}"
				fi
			;;
			esac
		fi
	  if test "$build_nrnpython_dynamic" = "no" ; then
		NRNPYTHON_LIBS="-lnrnpython $PYLIB"
		NRNPYTHON_LIBLA="../nrnpython/libnrnpython.la $PYLIB"
		NRNPYTHON_DEP="../nrnpython/libnrnpython.la"
		NRNPYTHON_INCLUDES="-I${PYINCDIR}"
		NRNPYTHON_EXEC="${ac_nrn_python}"
		build_nrnpython=yes
		if test "$CYGWIN" = "yes" ; then
			if test "$ac_nrn_cygwin" = "no" ; then
				CFLAGS="-mno-cygwin $CFLAGS"
			fi
		fi
		if test "$enable_bluegene" != yes ; then
			AC_NRN_RUNPYTHON
		fi
	  fi
		NRNPYTHON_PYLIBLINK="$PYLIBLINK"
		npy_NRNPYTHON_INCLUDES="-I${PYINCDIR}"
		build_nrnpython=yes

	fi
	if test "$CYGWIN" = "yes" ; then
		if test "$ac_nrn_cygwin" = "no" ; then
			CFLAGS="$nrn_temp_cflags"
		fi
	fi

	if test $NRNPYTHON_PYMAJOR -gt 2 ; then
		pypath=`dirname $NRNPYTHON_EXEC`
		if test -x $pypath/2to3 ; then
			PY2TO3=$pypath/2to3
		fi
	fi

	AC_SUBST(ac_pysetup)
	AC_SUBST(NRNPYTHON_LIBLA)
	AC_SUBST(NRNPYTHON_LIBS)
	AC_SUBST(NRNPYTHON_DEP)
	AC_SUBST(NRNPYTHON_INCLUDES)
	AC_SUBST(NRNPYTHON_DEFINES)
	AC_SUBST(NRNPYTHON_EXEC)
	AC_SUBST(NRNPYTHON_PYLIBLINK)
	AC_SUBST(setup_extra_link_args)
	AC_SUBST(NRNPYTHON_PYMAJOR)
	AC_SUBST(NRNPYTHON_PYVER)
	AC_SUBST(PY2TO3)
	AC_SUBST(PYTHON)
	AC_SUBST(npy_NRNPYTHON_INCLUDES)
	AC_SUBST(npy_apiver)
]) dnl end of AC_NRN_PYTHON
