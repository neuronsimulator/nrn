geosrc = $(top_srcdir)/share/lib/python/neuron/crxd/geometry3d

if BUILD_RX3D
all: graphicsPrimitives.cpp surfaces.cpp ctng.cpp
else
all:
endif

ctng.cpp: ctng.pyx
	$(CYTHON) $(geosrc)/ctng.pyx -o ctng.cpp

surfaces.cpp: surfaces.pyx marching_cubes2.c llgramarea.c
	$(CYTHON) $(geosrc)/surfaces.pyx -o surfaces.cpp


graphicsPrimitives.cpp: graphicsPrimitives.pyx
	$(CYTHON) $(geosrc)/graphicsPrimitives.pyx -o graphicsPrimitives.cpp

pkginclude_HEADERS = 
EXTRA_DIST = ctng.cpp ctng.pyx surfaces.cpp surfaces.pyx \
	graphicsPrimitives.cpp graphicsPrimitives.pyx \
	marching_cubes2.c llgramarea.c

if BUILD_RX3D
if BUILD_MINGW
BUILT_SOURCES = force setup.py ctng.cpp surfaces.cpp graphicsPrimitives.cpp
#"-c mingw32" only for python2 (do not use visual c 10)
mingwargs = @rxd_mingw_args@
force: ctng.cpp surfaces.cpp graphicsPrimitives.cpp
else
mingwargs =
install-exec-am: ctng.cpp surfaces.cpp graphicsPrimitives.cpp marching_cubes2.c llgramarea.c
endif
	if test ! -f graphicsPrimitives.cpp ; then \
		cp $(geosrc)/graphicsPrimitives.cpp  . ; \
	fi
	if test ! -f ctng.cpp ; then \
		cp $(geosrc)/ctng.cpp  . ; \
	fi
	if test ! -f surfaces.cpp ; then \
		cp $(geosrc)/surfaces.cpp  . ; \
	fi
	if test ! -f marching_cubes2.c ; then \
		cp $(geosrc)/marching_cubes2.c  . ; \
	fi
	if test ! -f llgramarea.c ; then \
		cp $(geosrc)/llgramarea.c  . ; \
	fi
#if BUILD_MINGW
	if ! grep -q '_hypot' ctng.cpp ; then \
		echo '#define _hypot hypot' > temp ; \
		cat ctng.cpp >> temp ; \
		mv temp ctng.cpp ; \
	fi
	if ! grep -q '_hypot' surfaces.cpp ; then \
		echo '#define _hypot hypot' > temp ; \
		cat surfaces.cpp >> temp ; \
		sed 's/EXTERN_C DL_IMPORT(\([^)]*\))/EXTERN_C \1/' temp > surfaces.cpp ; \
	fi
	if ! grep -q '_hypot' graphicsPrimitives.cpp ; then \
		echo '#define _hypot hypot' > temp ; \
		cat graphicsPrimitives.cpp >> temp ; \
		mv temp graphicsPrimitives.cpp ; \
	fi
#endif
	if test ! graphicsPrimitives.cpp -ef $(geosrc)/graphicsPrimitives.cpp ; then \
		cp $(geosrc)/*.py . ; \
	fi
	if test "$(PYTHON)" != "" ; then \
		$(PYTHON) setup.py build_ext $(mingwargs) --inplace > temp ; \
		if grep '\-shared.*\-lmsvcr90' temp ; then \
			grep '\-shared' temp | sed 's,\\,/,g' | sed 's,\-lmsvcr90,,' > temp1 ; \
			sh temp1 ; \
		fi ; \
 	fi
endif
# above grep avoids:
# A dynamic link library (DLL) initialization routine failed.
# for python2

clean-generic:
	rm -r -f build
	rm -f ctng.cpp graphicsPrimitives.cpp surfaces.cpp
	rm -fr *.so
