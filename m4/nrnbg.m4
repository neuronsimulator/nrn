AC_DEFUN([AC_NRN_BLUEGENE],[

AC_ARG_ENABLE([bluegene],
	AC_HELP_STRING([--enable-bluegene],[For BlueGene/L, supplies many extra configuration options]),[
	if test "$enable_bluegene" = "yes" ; then
		with_x="no"
		with_memacs="no"
		enable_shared="no"
		enable_pysetup="no"
		with_readline="no"
		java_dlopen="no"
		linux_nrnmech="no"
		if test x$with_nmodl_only != xyes ; then
			nmodl_build="no"
			if test "$BGLSYS" = "" ; then
				BGLSYS=/bgl/BlueLight/ppcfloor/bglsys
			fi
			if test "$CC" = "" ; then
				CC=blrts_xlc
			fi
			if test "$CXX" = "" ; then
				CXX=blrts_xlc++
			fi
			if test "$MPICC" = "" ; then
				MPICC=$CC
			fi
			if test "$MPICXX" = "" ; then
				MPICXX=$CXX
			fi
			if test "$OPTFLAGS" = "" ; then
				OPTFLAGS="-qarch=440d -qtune=440 -O3 -qstrict -qhot"
			fi
			if test "$CFLAGS" = "" ; then
				CFLAGS="$OPTFLAGS -g -I$BGLSYS/include"
			fi
			if test "$CXXFLAGS" = "" ; then
				CXXFLAGS=$CFLAGS
			fi
			if test "$BGL_LIBS" = "" ; then
				BGL_LIBS="-lmpich.rts -lmsglayer.rts -lrts.rts -ldevices.rts -ldevices.rts -L/opt/ibmcmp/xlmass/bg/4.3/blrts_lib -lmass"
			fi
			if test "$BG_CHECKPOINT" = "yes" ; then
                        	NRN_DEFINE(BLUEGENE_CHECKPOINT,1,[enable the checkpointing on BlueGene hardware])
                        	BGL_LIBS = "-lchkpt.rts $BGL_LIBS"
                	fi
			if test "$LIBS" = "" ; then
				LIBS="-L$BGLSYS/lib $BGL_LIBS"
			fi
			am_cv_CC_dependencies_compiler_type=xlc
			am_cv_CXX_dependencies_compiler_type=xlc
		else
			nmodl_build="yes"
		fi
		always_call_mpi_init=yes
		if test "$file_open_retry" = "" ; then
			file_open_retry=1
		fi
		NRN_DEFINE(BLUEGENE,1,[define if cross compiling for IBM BlueGene])
	fi
])

AC_ARG_ENABLE([bluegeneP],
	AC_HELP_STRING([--enable-bluegeneP],[For BlueGene/P, supplies many extra configuration options]),[
	if test "$enable_bluegeneP" = "yes" ; then
		with_x="no"
		with_memacs="no"
		enable_pysetup="no"
		enable_shared="no"
		with_readline="no"
		java_dlopen="no"
		linux_nrnmech="no"
		if test x$with_nmodl_only != xyes ; then
			nmodl_build="no"
			if test "$BG_BASE" = "" ; then
				BG_BASE="/bgsys/drivers/ppcfloor/"
			fi
			if test "$BG_INCLUDE" = "" ; then
				BG_INCLUDE="-I$BG_BASE/comm/include -I$BG_BASE/arch/include"
			fi
			if test "$PYINCDIR" = "" ; then
				PYINCDIR="$BG_BASE/gnu-linux/include/python2.6"
			fi
			if test "$PYLIB" = "" ; then
				PYLIB="-L$BG_BASE/gnu-linux/lib -lpython2.6"
				PYLIBDIR="$BG_BASE/gnu-linux/lib"
				PYLIBLINK="-L$BG_BASE/gnu-linux/lib -lpython2.6"
			fi
			if test "$LIBS" = "" ; then
				LIBS='-lmass'
			fi
			if test "$LDFLAGS" = "" ; then
				LDFLAGS='-qsmp -qnostaticlink'
			fi
			if test "$with_multisend" = "" ; then
				with_multisend=bgp
			fi
			if test "$CC" = "" ; then
				CC=mpixlc
			fi
			if test "$CXX" = "" ; then
				CXX=mpixlcxx
			fi
			if test "$MPICC" = "" ; then
				MPICC=$CC
			fi
			if test "$MPICXX" = "" ; then
				MPICXX=$CXX
			fi
			if test "$OPTFLAGS" = "" ; then
				OPTFLAGS="-O3 -qarch=450d"
			fi
			if test "$CFLAGS" = "" ; then
				CFLAGS="$OPTFLAGS $BG_INCLUDE"
			fi
			if test "$CXXFLAGS" = "" ; then
				CXXFLAGS=$CFLAGS
			fi
			if test "$BG_CHECKPOINT" = "yes" ; then
                        	NRN_DEFINE(BLUEGENE_CHECKPOINT,1,[enable the checkpointing on BlueGene hardware])
                        	BGL_LIBS = "-lchkpt.rts $BGL_LIBS"
                	fi
			if test "$deptype" = "" ; then
				am_cv_CC_dependencies_compiler_type=xlc
				am_cv_CXX_dependencies_compiler_type=xlc
			fi
		else
			nmodl_build="yes"
		fi
		always_call_mpi_init=yes
		if test "$file_open_retry" = "" ; then
			file_open_retry=1
		fi
		NRN_DEFINE(BLUEGENE,1,[define if cross compiling for IBM BlueGene L or P])
		NRN_DEFINE(BLUEGENEP,1,[define if cross compiling for IBM BlueGene/P])
	fi
])

AC_ARG_ENABLE([bgPlinux],
	AC_HELP_STRING([--enable-bgPlinux],[For BlueGene/P mpicc, supplies many extra configuration options]),[
	if test "$enable_bgPlinux" = "yes" ; then
		with_x="no"
		with_memacs="no"
#		enable_shared="no"
		enable_pysetup="no"
		with_readline="no"
		java_dlopen="no"
		linux_nrnmech="no"
		if test x$with_nmodl_only != xyes ; then
			nmodl_build="no"
			if test "$BG_BASE" = "" ; then
				BG_BASE="/bgsys/drivers/ppcfloor"
			fi
			BG_INCLUDE="-I$BG_BASE/comm/include -I$BG_BASE/arch/include"
			if test "$PYINCDIR" = "" ; then
				PYINCDIR="$BG_BASE/gnu-linux/include/python2.5"
			fi
			if test "$PYLIB" = "" ; then
				PYLIB="-L$BG_BASE/gnu-linux/lib -lpython2.5"
				PYLIBDIR="$BG_BASE/gnu-linux/lib"
				PYLIBLINK="-L$BG_BASE/gnu-linux/lib -lpython2.5"
			fi
			if test "$with_multisend" = "" ; then
				with_multisend=bgp
			fi
			if test "$CC" = "" ; then
				CC=mpicc
			fi
			if test "$CXX" = "" ; then
				CXX=mpicxx
			fi
			if test "$MPICC" = "" ; then
				MPICC=$CC
			fi
			if test "$MPICXX" = "" ; then
				MPICXX=$CXX
			fi
			if test "$OPTFLAGS" = "" ; then
				OPTFLAGS="-O3 -g"
			fi
			if test "$CFLAGS" = "" ; then
				CFLAGS="$OPTFLAGS $BG_INCLUDE"
			fi
			if test "$CXXFLAGS" = "" ; then
				CXXFLAGS=$CFLAGS
			fi
			if test "$BG_CHECKPOINT" = "yes" ; then
                        	NRN_DEFINE(BLUEGENE_CHECKPOINT,1,[enable the checkpointing on BlueGene hardware])
                        	BGL_LIBS = "-lchkpt.rts $BGL_LIBS"
                	fi
#			if test "$LDFLAGS" = "" ; then
#				LDFLAGS="-shared"
#			fi
		else
			nmodl_build="yes"
		fi
		always_call_mpi_init=yes
		if test "$file_open_retry" = "" ; then
			file_open_retry=1
		fi
		NRN_DEFINE(BLUEGENE,1,[define if cross compiling for IBM BlueGene L or P])
		NRN_DEFINE(BLUEGENEP,1,[define if cross compiling for IBM BlueGene/P])
	fi
])

AC_ARG_ENABLE([bluegeneQ],
        AC_HELP_STRING([--enable-bluegeneQ],[For BlueGene/Q, supplies many extra configuration options]),[
        if test "$enable_bluegeneQ" = "yes" ; then
                with_x="no"
                with_memacs="no"
                enable_shared="no"                
		enable_pysetup="no"
                with_readline="no"
                java_dlopen="no"
                linux_nrnmech="no"
                if test x$with_nmodl_only != xyes ; then
                        nmodl_build="no"
                        if test "$BG_BASE" = "" ; then
                                BG_BASE="/bgsys/drivers/ppcfloor/"
                        fi
                        if test "$BG_INCLUDE" = "" ; then
                                BG_INCLUDE="-I$BG_BASE/comm/include -I$BG_BASE/arch/include"
                        fi
			PYBASE=$BG_BASE/tools/python/bldsrc-2.6.7/Python-2.6.7
                        if test "$PYINCDIR" = "" ; then
                                PYINCDIR="$PYBASE/Include -I$PYBASE"
                        fi
                        if test "$PYLIB" = "" ; then
				PYLIB="-L$PYBASE -lpython2.6"
				PYLIBDIR="$PYBASE"
				PYLIBLINK="$PYLIB"
                        fi
                        if test "$LIBS" = "" ; then
                                LIBS='-lmass'
                        fi
                        if test "$LDFLAGS" = "" ; then
                                LDFLAGS='-qsmp -qnostaticlink'
                        fi
                        if test "$with_multisend" = "" ; then
                                with_multisend=yes
                        fi
                        if test "$CC" = "" ; then
                                CC=mpixlc
                        fi
                        if test "$CXX" = "" ; then
                                CXX=mpixlcxx
                        fi
                        if test "$MPICC" = "" ; then
                                MPICC=$CC
                        fi
                        if test "$MPICXX" = "" ; then
                                MPICXX=$CXX
                        fi
                        if test "$OPTFLAGS" = "" ; then
                                OPTFLAGS="-O3 -qarch=qp -q64 -qstrict -qnohot"
                        fi
                        if test "$CFLAGS" = "" ; then
                                CFLAGS="$OPTFLAGS $BG_INCLUDE"
                        fi
                        if test "$CXXFLAGS" = "" ; then
                                CXXFLAGS=$CFLAGS
                        fi
                        if test "$BG_CHECKPOINT" = "yes" ; then
                                NRN_DEFINE(BLUEGENE_CHECKPOINT,1,[enable the checkpointing on BlueGene hardware])
                                BGL_LIBS = "-lchkpt.rts $BGL_LIBS"
                        fi
                        if test "$deptype" = "" ; then
                                am_cv_CC_dependencies_compiler_type=xlc
                                am_cv_CXX_dependencies_compiler_type=xlc
                        fi
                else
                        nmodl_build="yes"
                fi
                always_call_mpi_init=yes
                if test "$file_open_retry" = "" ; then
                        file_open_retry=1
                fi
                NRN_DEFINE(BLUEGENE,1,[define if cross compiling for IBM BlueGene L or P])
                NRN_DEFINE(BLUEGENEQ,1,[define if cross compiling for IBM BlueGene/Q])
        fi
])

]) dnl end of AC_NRN_BLUEGENE
