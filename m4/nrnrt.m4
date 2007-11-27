AC_DEFUN([AC_NRN_REALTIME],[

AC_ARG_ENABLE([realtime],
	AC_HELP_STRING([--enable-realtime],[rtlinux dynamic clamp]),[
	if test "$enable_realtime" = "yes" ; then
		NRN_DEFINE(NRN_REALTIME,1,[define if want to use as rtlinux dynamic clamp])
		if test "$NRN_REALTIME_INCLUDE" = "" ; then
			NRN_REALTIME_INCLUDE="-I/home/hines/rtlinux/realtime/include"
		fi
		if test "$NRN_REALTIME_LIB" = "" ; then
			NRN_REALTIME_LIB="-L/home/hines/rtlinux/realtime/lib -llxrt -lpthread"
		fi
		LIBS="$LIBS $NRN_REALTIME_LIB"
	fi
])

AC_ARG_ENABLE([NI6229],
	AC_HELP_STRING([--enable-NI6229],
		[use National Instruments PCI-6229 DAQ])
,[
	NIDDK="/home/hines/rtlinux/ni/niddk"
	NRNNI_LIBS="-lni6229"
	NRNNI_LIBLA="../ni_pci_6229/libni6229.la"
	NRNNI_DEP="../ni_pci_6229/libni6229.la"
	NRN_DEFINE(NRN_6229, 1, [define if have National Instruments PCI-6229 DAQ])
],[
	NIDDK=""
	NRNNI_LIBLA=""
	NRNNI_LIBS=""
	NRNNI_DEP=""
]
)

AC_SUBST(NRN_REALTIME_INCLUDE)
AC_SUBST(NIDDK)
AC_SUBST(NRNNI_LIBLA)
AC_SUBST(NRNNI_LIBS)
AC_SUBST(NRNNI_DEP)
AM_CONDITIONAL(BUILD_6229, test x$enable_NI6229 = xyes)

]) dnl end of AC_NRN_REALTIME
