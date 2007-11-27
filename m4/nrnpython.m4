AC_DEFUN([AC_NRN_PYCONF],[
	dnl determine configuration if able to  run python
	ac_nrn_pyconf_val=""
	ac_nrn_pyconf_val=`$4 -c "import distutils.sysconfig
print distutils.sysconfig.get_config_var('[$2]')"`
	if test $? != 0 ; then
		AC_MSG_ERROR([could not run python in order to determine a
configuration variable.])
	fi
	if test "$ac_nrn_pyconf_val" = "" ; then
		[$1]=[$3]
	else
		[$1]=${ac_nrn_pyconf_val}
	fi
echo "[$2]  $ac_nrn_pyconf_val"
])

AC_DEFUN([AC_NRN_RUNPYTHON], [
	AC_MSG_CHECKING([if python include files and libraries work])
	zzzLD_LIBRARY_PATH=${LD_LIBRARY_PATH}
	LD_LIBRARY_PATH="${LD_LIBRARY_PATH}:${PYLIBDIR}"
	export LD_LIBRARY_PATH
	zzzCFLAGS="$CFLAGS"
	zzzLIBS="$LIBS"
	CFLAGS="$CFLAGS -I${PYINCDIR}"
	LIBS="${PYLIBLINK}"
	AC_TRY_RUN([
#include <Python.h>
int main() {
	Py_Initialize();
	Py_Finalize();
	return 0;
}
	],[
		AC_MSG_RESULT(yes)
	],[
		AC_MSG_ERROR(could not run a test that used the python library.
Examine config.log to see error details. Something wrong with
	PYLIB=$PYLIB
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
	build_nrnpython=no

	AC_ARG_WITH([nrnpython],
		AC_HELP_STRING([--with-nrnpython=[desired python binary]],
			[Python interpreter can be used (default is NO)
Probably need to set PYLIBDIR to find libpython...
and PYINCDIR to find Python.h
]),
		[ac_nrn_python=$withval], [ac_nrn_python=no]
	)

	AC_ARG_ENABLE([numpy],
		AC_HELP_STRING([--enable-numpy],
			[allow use of numpy (disabled by default) if python
enabled.
]),
		[ac_nrn_numpy=$enableval], [ac_nrn_numpy=no]
	)

	if test "$ac_nrn_python" = "yes" ; then
		ac_nrn_python="python"
	fi


	if test "$ac_nrn_python" != "no" ; then
		
		ac_nrn_python=`which ${ac_nrn_python}`

		if test "$ac_nrn_python" = "" ; then

			AC_MSG_ERROR([Either python is not in the path or the specified python does not exist.])

		fi

		echo "Python binary found ($ac_nrn_python)"

		AC_MSG_CHECKING([nrnpython configuration])
		NRN_DEFINE(USE_PYTHON,1,[define if Python available])
		if test "$PYVER" = "" ; then
			AC_NRN_PYCONF(xxx,VERSION,2.4,$ac_nrn_python)
			PYVER=python${xxx}
		fi
		if test "$PYINCDIR" = "" ; then
			AC_NRN_PYCONF(xxx,INCLUDEDIR,"",$ac_nrn_python)
			if test "$xxx" = "" ; then
				PYINCDIR="$HOME/python/include/${PYVER}"
			else
				PYINCDIR="${xxx}/${PYVER}"
			fi
		fi
		if test "$EXTRAPYLIBS" = "" ; then
			AC_NRN_PYCONF(EXTRAPYLIBS,LIBS,"",$ac_nrn_python)
		fi
		setup_extra_link_args=extra_link_args
		case "$host_os" in
			darwin*)
				setup_extra_link_args='#extra_link_args'
				;;
		esac
		if test "$PYLIB" = "" ; then
			case "$host_os" in
			darwin*)
				AC_NRN_PYCONF(xxx,LINKFORSHARED,"",$ac_nrn_python)
				PYLIB="$xxx"
				PYLIBLINK="$PYLIB"
				;;
			*)
				AC_NRN_PYCONF(xxx,LINKFORSHARED,"",$ac_nrn_python)
				PYLINKFORSHARED="$xxx"				
				AC_NRN_PYCONF(xxx,LIBDEST,"",$ac_nrn_python)
				if test "$xxx" = "" ; then
					PYLIBDIR="$HOME/python/lib"
				else
					PYLIBDIR="${xxx}/config"
				fi
PYLIB="-L${PYLIBDIR} -l${PYVER} ${EXTRAPYLIBS} ${PYLINKFORSHARED} -R${PYLIBDIR}"
PYLIBLINK="-L${PYLIBDIR} -l${PYVER} ${EXTRAPYLIBS} ${LIBS}"
			;;
			esac
		fi
		NRNPYTHON_LIBS="-lnrnpython $PYLIB"
		NRNPYTHON_LIBLA="../nrnpython/libnrnpython.la $PYLIB"
		NRNPYTHON_DEP="../nrnpython/libnrnpython.la"
		NRNPYTHON_INCLUDES="-I${PYINCDIR}"


		if test "$ac_nrn_numpy" = "yes" ; then
			AC_MSG_CHECKING([numpy availability])
			CMD="import numpy;print numpy.__path__@<:@0@:>@ + '''/core/include''' "
			PYTHON_NUMPY_INCLUDE=`${ac_nrn_python} -c "${CMD}"`
			if test "$PYTHON_NUMPY_INCLUDE" != ""; then
				HAVE_NUMPY="yes"
				NRNPYTHON_INCLUDES="${NRNPYTHON_INCLUDES} -I${PYTHON_NUMPY_INCLUDE}"
				NRNPYTHON_DEFINES="-DWITH_NUMPY"
			else
				AC_MSG_ERROR([Python cannot import numpy (numpy not installed?).])
			fi
		else
			echo "numpy not enabled. If desired add --enable-numpy to configure."
			HAVE_NUMPY="no"
			NRNPYTHON_DEFINES="-UWITH_NUMPY"
		fi


		NRNPYTHON_EXEC="${ac_nrn_python}"
		build_nrnpython=yes
		if test "$enable_bluegene" != yes ; then
			AC_NRN_RUNPYTHON
		fi
	fi
	AC_SUBST(NRNPYTHON_LIBLA)
	AC_SUBST(NRNPYTHON_LIBS)
	AC_SUBST(NRNPYTHON_DEP)
	AC_SUBST(NRNPYTHON_INCLUDES)
	AC_SUBST(NRNPYTHON_DEFINES)
	AC_SUBST(NRNPYTHON_EXEC)
	AC_SUBST(setup_extra_link_args)

]) dnl end of AC_NRN_PYTHON
