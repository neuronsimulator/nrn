dnl --with-nrnjava is the default and means that libnrnjava.so will
dnl be built so that neuron can be dlopened by java

AC_DEFUN([AC_JVM2_CREATE], [
AC_MSG_CHECKING([checking whether the java 2 virtual machine can be created])
xLD_LIBRARY_PATH=${LD_LIBRARY_PATH}
LD_LIBRARY_PATH="${LD_LIBRARY_PATH}:${JLIBDIR}:${JVMLIBDIR}"
export LD_LIBRARY_PATH
zzzCFLAGS=$CFLAGS
zzzLIBS=$LIBS
CFLAGS="$CFLAGS $JCFLAGS"
LIBS="$JLFLAGS $LIBS"
AC_TRY_RUN([
#include <stdlib.h>
#include <jni.h>
int main() {
	jint res;
	void* jenv;
	JavaVM* jvm;
	JavaVMInitArgs args;
	args.version = JNI_VERSION_1_2;
	args.nOptions = 0;
	args.options = 0;
	args.ignoreUnrecognized = JNI_FALSE;
	res = JNI_CreateJavaVM(&jvm, &jenv, &args);
	printf("JNI_CreateJavaVM returned %d\n", res);
	if (res < 0) {
		exit(res);
	}else{
		exit(0);
	}
	return 0;
}
],[
echo "The present configuration allows initialization of the java 2 virtual"
echo " machine to succeed."
jvmcreate=yes
],[
echo "The test program that tests initialization of the java 2 virtual machine,"
echo " failed. See config.log ."
echo "  It may be that linking requires some more -l libraries"
echo "  or extended LD_LIBRARY_PATH"
echo "  or perhaps the java vm is not JNI_VERSION_1_2."
jvmcreate=no
],[
echo "cross compiling not allowed."
exit 1
])
echo "JCFLAGS=${JCFLAGS}"
echo "JLFLAGS=${JLFLAGS}"
CFLAGS=$zzzCFLAGS
LIBS=$zzzLIBS
LD_LIBRARY_PATH=${xLD_LIBRARY_PATH}
export LD_LIBRARY_PATH
]) dnl end of AC_JVM2_CREATE

AC_DEFUN([AC_JVM1_CREATE], [
AC_MSG_CHECKING([checking whether the java 1 virtual machine can be created])
xLD_LIBRARY_PATH=${LD_LIBRARY_PATH}
LD_LIBRARY_PATH="${LD_LIBRARY_PATH}:${JLIBDIR}:${JVMLIBDIR}"
export LD_LIBRARY_PATH
zzzCFLAGS=$CFLAGS
CFLAGS="$CFLAGS $JCFLAGS $JLFLAGS"
AC_TRY_RUN([
#include <stdlib.h>
#include <jni.h>
int main() {
	jint res;
	void* jenv;
	JavaVM* jvm;
	JDK1_1InitArgs args;
	args.version = 0x00010001;
	JNI_GetDefaultJavaVMInitArgs(&args);
	res = JNI_CreateJavaVM(&jvm, &jenv, &args);
	printf("JNI_CreateJavaVM returned %d\n", res);
	if (res < 0) {
		exit(res);
	}else{
		exit(0);
	}
	return 0;
}
],[
echo "The present configuration allows initialization of the java 1 virtual"
echo " machine to succeed."
jvmcreate=yes
],[
echo "The test program that tests initialization of the java 1 virtual machine,"
echo " failed. See config.log ."
echo "  It may be that linking requires some more -l libraries"
echo "  or extended LD_LIBRARY_PATH"
echo "  or perhaps the java vm is not 0x00010001."
jvmcreate=no
],[
echo "cross compiling not allowed."
exit 1
])
echo "JCFLAGS=${JCFLAGS}"
echo "JLFLAGS=${JLFLAGS}"
CFLAGS=$zzzCFLAGS
LD_LIBRARY_PATH=${xLD_LIBRARY_PATH}
export LD_LIBRARY_PATH
]) dnl end of AC_JVM1_CREATE

AC_DEFUN([AC_NRN_JAVA], [

if test "$with_nrnjava" = "" ; then
  with_nrnjava=no			# Maybe someday it will be the default
fi

dnl however if neosim is requested then nrnjava is not optional

if test "$with_neosim" = "" ; then
  with_neosim=no
fi

AC_ARG_WITH([neosim],
AC_HELP_STRING([--with-neosim],[Compile with USENEOSIM defined])
AC_HELP_STRING([--without-neosim],[This is the default])
,[
	if test "[$with_neosim]" = "yes" ; then
		echo "Allow use of neosim. This also forces with-nrnjava."
		NRN_DEFINE([USENEOSIM], 1, [define if want to use NEOSIM])
		with_nrnjava=yes
	else
		echo "Do not compile neosim specific code."
	fi
])dnl

AC_ARG_ENABLE([ncs],
AC_HELP_STRING([--enable-ncs],[Compile with USENCS defined - Allows NCS to use NEURON])
AC_HELP_STRING([--disable-ncs],[This is the default])
,[
	if test "$enable_ncs" = "yes" ; then
		if test "[$with_neosim]" = "yes" ; then
AC_MSG_ERROR([Cannot combine enable-ncs and with-neosim])
		else
			echo "Allow NCS to use NEURON."
NRN_DEFINE([USENCS], 1, [define if want to be usable by NCS])
		fi
	else
		echo "Do not compile NCS specific code."
	fi
])dnl

AC_ARG_WITH(nrnjava,
AC_HELP_STRING([--with-nrnjava],[Must explicitly use it if you want it.
                             Enable the java interface. This builds
                             libnrnjava.so and makes the NrnJava
                             class available in hoc.])
AC_HELP_STRING([--without-nrnjava],[No java interface. This is the default])
,[
	if test "$with_nrnjava" = "no" ; then
		dnl take away with the left what has been given with the right
		echo "Not building the nrnjava interface"
		build_nrnjava=no
	else
		build_nrnjava=yes
	fi
])

if test "$build_nrnjava" = "yes" ; then
	echo "Build the nrnjava interface"
dnl EXEEXT is used by java tests and that does not work if not using C
AC_LANG_PUSH([C])

if test "$JAVAC" = "" ; then
	JAVAC=javac
fi
if test "$JAVA" = "" ; then
	JAVA=java
fi
if test "$JAVAH" = "" ; then
	JAVAH=javah
fi

if test "$JNI_INCLUDE_FLAGS" = "" ; then
	AC_JNI_INCLUDE_DIR
	JNI_INCLUDE_DIR=""
	for JNI_INCLUDE_DIR in $JNI_INCLUDE_DIRS ; do
		JNI_INCLUDE_FLAGS="$JNI_INCLUDE_FLAGS -I$JNI_INCLUDE_DIR"
	done
	JDKDIR="$_JTOPDIR"
fi

echo "JDKDIR=$JDKDIR"

jvm_do_test=yes
if test $host_os = cygwin ; then
	jvm_do_test=no
	NRN_DEFINE(USEJVM,2,[1 then version 0x00010001, 2 then JNI_VERSION_1_2])
	NRN_DEFINE(USENRNJAVA,1,[if 1 then NrnJava will be a class in hoc])
fi

if test "$JVM_LIB_FLAGS" = "" ; then
	case "$host_os" in
	  darwin*)
		JVM_LIB_FLAGS="-framework JavaVM"
		JVM_RPATH=""
		;;
	  cygwin*)
		JVM_LIB_FLAGS=""
		JVM_RPATH=""
		;;
	  *)
		if test -z "$JVMLIBDIR" ; then
			if test -z "$JDKDIR" ; then
				echo "Cannot determine JVMLIBDIR without JDKDIR"
				exit 1
			fi
			xjava=`find $JDKDIR/jre -name libjvm\* -print|sed -n 1p`
			echo "Found a path to the libjvm"
			echo "$xjava"
			xjava=`echo $xjava | sed -n 's;\(.*\)/libjvm.*;\1;p'`
			JVMLIBDIR=$xjava
			echo "Determined JVMLIBDIR=$JVMLIBDIR"
		fi
		JLIBDIR=`echo $JVMLIBDIR | sed 's;/[[^/]]*$;;'`
		echo "JLIBDIR=$JLIBDIR"
		JVM_LIB_FLAGS="-L$JLIBDIR -L$JVMLIBDIR -ljvm -lpthread"
		JVM_RPATH="-R $JLIBDIR -R $JVMLIBDIR"
		;;
	esac
fi

echo "Using JNI_INCLUDE_FLAGS=\"${JNI_INCLUDE_FLAGS}\""
echo "Using JVM_LIB_FLAGS=\"${JVM_LIB_FLAGS}\""

dnl Test if the java virtual machine version 2 can be created
dnl returns jvmcreate as yes or no

if test $jvm_do_test = "yes" ; then

JCFLAGS="$JNI_INCLUDE_FLAGS"
JLFLAGS="$JVM_LIB_FLAGS"
AC_JVM2_CREATE
if test "$jvmcreate" = "no" ; then
	JCFLAGS="$JNI_INCLUDE_FLAGS"
	JLFLAGS="$JVM_LIB_FLAGS"
	AC_JVM1_CREATE
	if test "$jvmcreate" = "no" ; then
echo "Assuming that the test failed because"
echo "JVMLIBS=\"${JVMLIBS}\""
echo "is missing some paths and/or libraries", look
echo "for examples of machine and java specific values for JVMLIBS in"
echo "ftp://ftp.neuron.yale.edu/neuron/unix/jvmlibs"
echo "If you don't see yours in the list and you figure out the"
echo "correct value send it to me at michael.hines@yale.edu."
		exit 1
	else
NRN_DEFINE(USEJVM,1,[1 then version 0x00010001, 2 then JNI_VERSION_1_2])
NRN_DEFINE(USENRNJAVA,1,[if 1 then NrnJava will be a class in hoc])
	fi
else
NRN_DEFINE(USEJVM,2,[1 then version 0x00010001, 2 then JNI_VERSION_1_2])
NRN_DEFINE(USENRNJAVA,1,[if 1 then NrnJava will be a class in hoc])
fi
fi

if test "$java_dlopen" = "yes" ; then
	AC_MSG_NOTICE([Use the dlopen technique to load the jvm])
	NRN_DEFINE(JVM_DLOPEN,1,[if defined then try to dlopen the jvm])
	NRNJAVA_LIBS="-lnrnjava"
	NRNJAVA_LIBLA="../nrnjava/libnrnjava.la"
else
	NRNJAVA_LIBS="-lnrnjava $JLFLAGS $JVM_RPATH"
	NRNJAVA_LIBLA="../nrnjava/libnrnjava.la $JLFLAGS $JVM_RPATH"
fi
NRNJAVA_DEP="../nrnjava/libnrnjava.la"

AC_CHECK_CLASSPATH
AC_PROG_JAVAC
AC_PROG_JAVA

AC_SUBST(JAVAH)
AC_SUBST(JDKDIR)
AC_SUBST(JNI_INCLUDE_FLAGS)
AC_LANG_POP([])
else
	NRNJAVA_LIBS=""
	NRNJAVA_LIBLA=""
	NRNJAVA_DEP=""
fi
AC_SUBST(NRNJAVA_LIBLA)
AC_SUBST(NRNJAVA_LIBS)
AC_SUBST(NRNJAVA_DEP)

])
