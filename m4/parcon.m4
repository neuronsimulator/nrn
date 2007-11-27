AC_DEFUN([AC_NRN_ParallelContext],[
xLIBS="$LIBS"
dnl does PVM exist. specifically pvm3.h
if test "$use_mpi" = "no" ; then
AC_CHECK_HEADER($PVM_ROOT/include/pvm3.h,
[
	PVM_INCLUDES='-I${PVM_ROOT}/include'
	PVM_XTRA_LIBS="`sed -n '/^ARCHLIB/s/[[^-]]*//p' ${PVM_ROOT}/conf/${PVM_ARCH}.def`"
	echo "PVM_XTRA_LIBS=$PVM_XTRA_LIBS"
	PVM_LIBS='-L${PVM_ROOT}/lib/${PVM_ARCH} -lpvm3'
	tPVM_LIBS="-L${PVM_ROOT}/lib/${PVM_ARCH} -lpvm3 ${PVM_XTRA_LIBS}"
	LIBS="$tPVM_LIBS $LIBS"
	AC_CHECK_LIB(pvm3,pvm_parent,[
		NRN_DEFINE(HAVE_PVM3_H,1,[define if pvm3.h exists])
		AC_CHECK_LIB(pvm3,pvm_pkmesg,[
			NRN_DEFINE(HAVE_PKMESG,1,[define if pvm_pkmesg in the -lpvm3 library])
		])
	],[
		echo "There is a pvm3.h but I can't link with -lpvm3"
		echo "So not using pvm afterall."
		PVM_INCLUDES=""
		PVM_LIBS=""
		tPVM_LIBS=""
		case "$host_os" in
		irix6* )
echo "This is a $host so if you want to use ParallelContext under PVM"
echo "you probably need to rebuild"
echo "everything (including interviews) with the environment variables"
			if test "$GCC"='yes' ; then
				echo setenv CC '"'"$CC -mabi=64"'"'
			else
				echo setenv CC '"'"$CC -64"'"'
			fi
			if test "$GXX"='yes' ; then
				echo setenv CXX '"'"$CXX -mabi=64"'"'
			else
				echo setenv CXX '"'"$CXX -64"'"'
			fi
		;;
		esac
	])
	LIBS="$xLIBS"
])
else dnl use_mpi is not "no" so avoid pvm
	PVM_INCLUDES=""
	PVM_LIBS=""
	tPVM_LIBS=""
	PVM_XTRA_LIBS=""
fi

dnl Check for SIGPOLL and verify that it works
echo
AC_LANG_PUSH([C++])
AC_TRY_RUN([
#include <stdio.h>
#include <unistd.h>
#include <stropts.h>
#include <signal.h>
#include <errno.h>
int main() {
	int fd, arg;
	sighold(SIGPOLL);
	fd = fileno(popen("ls", "r"));
	if (ioctl(fd, I_GETSIG, &arg) < 0 && errno != EINVAL) {
		exit(1);
	}
	exit(0);
	return 0;
}
],[
NRN_DEFINE(HAVE_SIGPOLL,1,[define if SIGPOLL in signal.h])
echo "do SIGPOLL and I_GETSIG work: yes"
],[
echo "do SIGPOLL and I_GETSIG work: no"
echo " But see config.log to see why it failed."
],[
echo "cross compiling: Assume HAVE_SIGPOLL is 0"
])
AC_LANG_POP([])

dnl Check for STL
AC_CXX_HAVE_STL

AC_SUBST(PVM_INCLUDES)
AC_SUBST(PVM_XTRA_LIBS)
AC_SUBST(PVM_LIBS)

])dnl end of AC_NRN_ParallelContext

