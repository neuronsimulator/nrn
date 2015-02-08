AC_DEFUN([AC_NRN_WITH_MPI],[

MPICCnrnmpi=$CC

AC_ARG_WITH(mpi,
AC_HELP_STRING([--with-mpi],[Also compile the parallel code in src/sundials/shared])
AC_HELP_STRING([--without-mpi],[This is the default. Do not compile the parallel sundials code])
,[
	if test "$with_mpi" = "yes" ; then
		ACX_MPI([
			use_mpi=yes
			NRN_DEFINE(NRNMPI,1,[Define if you want MPI specific features.])
			AC_LANG_PUSH([C++])
			ACX_MPI([use_mpi=yes])
			AC_LANG_POP()
dnl The following is the only way I can figure out how to use mpicc or mpicxx
dnl since libtool complains when CC=mpicc that there is no tag.
dnl Therefore when mpicc is desired a LIBTOOLTAG = --tag=CC is put into
dnl the Makefile.am. Hopefully the following will always take precedence
dnl over the AC_SUBST in AC_PROC_LIBTOOL
dnl The latest version of automake and libtool now seem to handle tags
dnl so we commnt out the next two lines. However we still need it
dnl for nrnivmodl.
dnl LIBTOOL='$(SHELL) $(top_builddir)/libtool $(LIBTOOLTAG)'
dnl AC_SUBST(LIBTOOL)
		    if test "$nrnmpi_dynamic" = "no" ; then
			CC=$MPICC
			CXX=$MPICXX
		    fi
			MPICCnrnmpi=$MPICC
			LIBTOOLTAG='--tag=CC'
			dnl AC_SUBST(CC)
			dnl AC_SUBST(CXX)
			AC_SUBST(LIBTOOLTAG)
		],[
			AC_MSG_ERROR([Cannot compile MPI program])
		])
	else
		use_mpi=no
	fi
],[
		use_mpi=no
])

])dnl end of AC_NRN_WITH_MPI

AC_DEFUN([AC_NRN_PARANEURON],[

AC_ARG_WITH(paranrn,
AC_HELP_STRING([--with-paranrn],[parallel variable step integration for individual cells])
AC_HELP_STRING([--with-paranrn=dynamic],[as above, but with dynamic loading of mpi shared libaries])
AC_HELP_STRING([--without-paranrn],[default. No parallel integration])
,[
	nrnmpi_dynamic="no"
	if test "$with_paranrn" = "dynamic" ; then
		with_paranrn=yes
		nrnmpi_dynamic="yes"
		NRN_DEFINE(NRNMPI_DYNAMICLOAD,1,[define to 1 if you want mpi dynamically loaded instead of linked normally])
	fi
	if test "$with_paranrn" = "yes" ; then
		with_mpi=yes
		use_paranrn=yes
		NRN_DEFINE(PARANEURON,1,[Define if allowing distributed variable step integration])
	else
		use_paranrn=no
	fi
],[
	nrnmpi_dynamic="no"
	use_paranrn=no
])

AC_ARG_WITH(multisend,
AC_HELP_STRING([--with-multisend],[Allow optional MPI_ISend/Recv for spike transfer])
AC_HELP_STRING([--with-multisend=bgp],[Use BlueGene/P style dma spike transfer])
AC_HELP_STRING([--with-multisend=all],[Allow ISend or BGP style dma])
AC_HELP_STRING([--with-multisend=allreplay],[Allow ISend or BGP style dma, and with possiblity of Record/Replay])
,[
	if test "$with_multisend" = "yes" ; then
		with_mpi=yes
		use_bgpdma=yes
		NRN_DEFINE(BGPDMA,1,
[Define bits for BGPDMA & 1 (ISend) & 2 (DMA spike transfer) & 4 (DMA Record Replay)])
	elif test "$with_multisend" = "bgp" ; then
		with_mpi=yes
		use_bgpdma=yes
		NRN_DEFINE(BGPDMA,2,[Define bits for BGPDMA & 1 (ISend) & 2 (DMA spike transfer) & 4 (DMA Record Replay)])
	elif test "$with_multisend" = "all" ; then
		with_mpi=yes
		use_bgpdma=yes
		NRN_DEFINE(BGPDMA,3,[Define bits for BGPDMA & 1 (ISend) & 2 (DMA spike transfer) & 4 (DMA Record Replay)])
	elif test "$with_multisend" = "allreplay" ; then
		with_mpi=yes
		use_bgpdma=yes
		NRN_DEFINE(BGPDMA,7,[Define bits for BGPDMA & 1 (ISend) & 2 (DMA spike transfer) & 4 (DMA Record Replay)])
	else
		use_bgpdma=no
		NRN_DEFINE(BGPDMA,0,[Define bits for BGPDMA & 1 (ISend) & 2 (DMA spike transfer) & 4 (DMA Record Replay)])
	fi
	if test "$use_bgpdma" = "yes" ; then
		NRN_DEFINE(USE_NRNFILEWRAP,1,[Define to allow possibility of rank 0 read file into buffer and broadcast to all ranks.])
	fi
],[
	use_bgpdma=no
])

])dnl end of AC_NRN_PARANEURON

AC_DEFUN([AC_NRN_MUSIC],[
AC_ARG_WITH(music,
AC_HELP_STRING([--with-music=/prefix/of/music/installation],[Want to use the  MUSIC - MUlti SImulation Coordinator])
,[
	if test ! -f "$with_music/bin/music" ; then
		AC_MSG_ERROR([MUSIC is not installed at the indicated location])
	fi
	if test x$with_nrnpython = x$no ; then
		AC_MSG_ERROR([--with-music can be used only if --with-nrnpython is also used.])
	fi
	with_mpi=yes
	with_paranrn=yes
	use_music=yes
	NRN_DEFINE(NRN_MUSIC,1,[Define if you want the MUSIC - MUlti SImulation Coordinator])
	MUSIC_LIBLA="$with_music/lib/libmusic.la"
	MUSIC_INCDIR="$with_music/include"
	MUSIC_LIBDIR="$with_music/lib"
],[
	use_music=no
	MUSIC_INCDIR='.'
	MUSIC_LIBDIR='.'
])
AM_CONDITIONAL(BUILD_NEURONMUSIC, test x$use_music = xyes)
AC_SUBST(MUSIC_LIBLA)
AC_SUBST(MUSIC_INCDIR)
AC_SUBST(MUSIC_LIBDIR)
])dnl end of AC_NRN_MUSIC

AC_DEFUN([AC_NRN_WITH_METIS],[

AC_ARG_WITH(metis,
AC_HELP_STRING([--with-metis],[use METIS to distribute cell equations among processors])
AC_HELP_STRING([--without-metis],[default.])
,[
	if test "$with_metis" != "no" ; then
		use_metis=yes
		NRN_DEFINE(METIS,1,[Define if allowing distributed variable step integration])
		if test "$with_metis" != "yes" ; then
			METISDIR="$with_metis"
		else
			METISDIR=/home/hines/metis-4.0
		fi
		METISINCLUDE="-I$METISDIR/Lib"
		METISLIB="-L$METISDIR -lmetis"
		METISLIBADD="$METISDIR/libmetis.a"
		zzzCFLAGS="$CFLAGS"
		zzzLIBS="$LIBS"
		CFLAGS="$CFLAGS $METISINCLUDE"
		LIBS="$METISLIB $LIBS"
		AC_LANG_PUSH(C)
		AC_LINK_IFELSE([AC_LANG_SOURCE([
#include <stdio.h>
#include <metis.h>
int main() {
	sizeof(idxtype) == sizeof(int);
	return 0;
}
		])],[
			if grep 'define *IDXTYPE_INT' $METISDIR/Lib/struct.h ; then
				NRN_DEFINE(NRNIDXTYPE,int,[Define int or short depending on idxtype in metis struct.h])
			else
				NRN_DEFINE(NRNIDXTYPE,short,[Define int or short depending on idxtype in metis struct.h])
			fi
		],[
			AC_MSG_ERROR([Could not compile or link metis])
		])
		AC_LANG_POP(C)
		CFLAGS="$zzzCFLAGS"
		LIBS="$zzzLIBS"
	else
		use_metis=no
	fi
],[
	use_metis=no
])
AC_SUBST(METISINCLUDE)
AC_SUBST(METISOBJADD)
AC_SUBST(METISLIBADD)
AC_SUBST(METISLIB)
AC_SUBST(MPICCnrnmpi)

])dnl end of AC_NRN_WITH_METIS

