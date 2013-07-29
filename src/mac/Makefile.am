# This makefile copiessome .c and .h files into
# the distribution tar file and installs the mac osx launch scripts

S =  modlunit.sh mknrndll.sh nrngui.sh neurondemo.sh idraw.sh mos2nrn.sh

launch_scripts = macnrn.term $(S)

EXTRA_DIST = maccmd.c njconf.h nrnneosm.h bbsconf.h macnrn.h nrnconf.h \
	njvmstub.c nrncarbon.h \
	$(launch_scripts) launch_inst.sh \
	modlunit.info mknrndll.info nrngui.info neurondemo.info idraw.info mos2nrn.info \
	modlunit.icns mknrndll.icns nrngui.icns neurondemo.icns idraw.icns mos2nrn.icns \
	mkdmg.sh after-install.sh launch.c mac2uxarg.c

host_cpu = @host_cpu@

if MAC_DARWIN
carbon = @enable_carbon@
bin_SCRIPTS = $(launch_scripts)
install: install-am
if UniversalMacBinary
	$(CC) -arch ppc -o aoutppc -Dcpu="\"$(host_cpu)\"" -I. $(srcdir)/launch.c $(srcdir)/mac2uxarg.c -framework Carbon
	$(CC) -arch i386 -o aouti386 -Dcpu="\"$(host_cpu)\"" -I. $(srcdir)/launch.c $(srcdir)/mac2uxarg.c -framework Carbon
	lipo aouti386 aoutppc -create -output a.out
else
	gcc -g -arch i386 -Dncpu="\"$(host_cpu)\"" -I. $(srcdir)/launch.c $(srcdir)/mac2uxarg.c -framework Carbon

endif
	carbon=$(carbon) sh $(srcdir)/launch_inst.sh "$(host_cpu)" "$(DESTDIR)$(prefix)" "$(srcdir)"
	for i in $(S) ; do \
		sed "s/^CPU.*/CPU=\"$(host_cpu)\"/" < $(DESTDIR)$(bindir)/$$i > temp; \
		mv temp $(DESTDIR)$(bindir)/$$i; \
		chmod 755 $(DESTDIR)$(bindir)/$$i; \
	done
endif
