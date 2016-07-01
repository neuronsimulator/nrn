# This file is part of MXE.
# See index.html for further information.

PKG             := neuron
$(PKG)_IGNORE   := 
$(PKG)_VERSION  := 7.5
$(PKG)_CHECKSUM := f45c9af9ed8210df9dc4cbf8bfebc7039a406041d5439d8137fb6f6b1451001f
$(PKG)_SUBDIR   := nrn-7.5
$(PKG)_FILE     := nrn-$($(PKG)_VERSION).alpha-1439+.tar.gz
#$(PKG)_FILE     := nrn-$($(PKG)_VERSION).rel-1390.tar.gz
$(PKG)_URL      := http://www.neuron.yale.edu/ftp/neuron/versions/alpha/$($(PKG)_FILE)
$(PKG)_DEPS     := gcc readline nrniv pthreads termcap


PATH_TO_HOST_NEURON := $(PREFIX)/share/$($(PKG)_SUBDIR)

define $(PKG)_UPDATE
    wget -q -O- 'http://www.neuron.yale.edu/neuron/download/getstd' | \
    $(SED) -n 's_.*>[0-9]\./nrn-\([0-9]\.[0-9]\.[0-9]\).*tar.gz">_\1_ip' | \
    head -1
endef


define $(PKG)_BUILD

	### This is neeeded because nmodl/nocmodl needs to run on host
	if [[ ! -d $(PATH_TO_HOST_NEURON) ]] ; then \
		echo "Built host neuron and nocmodl in $(PATH_TO_HOST_NEURON)";  \
		( cd $$(dirname $(PATH_TO_HOST_NEURON)) && tar xf $(PWD)/pkg/$($(PKG)_FILE) ); \
		( cd $(PATH_TO_HOST_NEURON) && \
			./configure --with-nmodl-only --without-x && \
			$(MAKE)                         ) ; \
	fi

	## http://stackoverflow.com/questions/5212454/allegro-question-how-can-i-get-rid-of-the-cmd-window
	## ‐Wl,‐‐subsystem,windows
	cd '$(1)' CXXFLAGS=-mwindows LDFLAGS=‐‐subsystem=windows && ./configure \
		--prefix='$(PREFIX)/$(TARGET)' \
		--with-paranrn=dynamic \
		--with-nrnpython=dynamic \
		MPICC=x86_64-w64-mingw32.shared-gcc \
		MPICXX=x86_64-w64-mingw32.shared-g++ \
		CFLAGS='-I/home/hines/mxe/ms-mpi/include' \
		MPILIBS='-L/home/hines/mxe/ms-mpi/lib/x64 -lmsmpi' \
		LIBS='-L/home/hines/mxe/ms-mpi/lib/x64 -lmsmpi' \
		PYINCDIR=/home/hines/mxe/Python27/include \
		PYLIBDIR=/home/hines/mxe/Python27/libs \
		PYLIBLINK='-L/home/hines/mxe/Python27/libs -lpython27' \
		PYLIB='-L/home/hines/mxe/Python27/libs -lpython27' \
		--build="`config.guess`" \
		--host='$(TARGET)' \
		--with-iv='$(PREFIX)/$(TARGET)'

	### nocmodl of host (not the target one) ###
	$(SED) -i 's#^NMODL = ../nmodl/nocmodl#NMODL = $(PATH_TO_HOST_NEURON)/src/nmodl/nocmodl#' '$(1)'/src/nrnoc/Makefile 
	#$(SED) -i '/^am_ivoc_OBJECTS/ s#\\$$# ivocwin.$$\(OBJEXT\) \\#' '$(1)'/src/ivoc/Makefile
	$(SED) -i  's#^\tg++ #\t$$\(CXX\) #g' '$(1)'/src/nrniv/Makefile

	$(MAKE) -C '$(1)' 
	#$(MAKE) -C '$(1)' install 
	exit 1
endef

