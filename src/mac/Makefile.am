# This makefile copiessome .c and .h files into
# the distribution tar file and installs the mac osx launch scripts

S =  modlunit.sh mknrndll.sh nrngui.sh neurondemo.sh idraw.sh mos2nrn.sh

launch_scripts = macnrn.term $(S)

EXTRA_DIST = \
	$(launch_scripts) launch_inst.sh \
	modlunit.icns mknrndll.icns nrngui.icns neurondemo.icns idraw.icns mos2nrn.icns \
	mkdmg.sh after-install.sh prototype_applescript.txt

host_cpu = @host_cpu@

if MAC_DARWIN
bin_SCRIPTS = $(launch_scripts)
install: install-am
	sh $(srcdir)/launch_inst.sh "$(host_cpu)" "$(DESTDIR)$(prefix)" "$(srcdir)"
	for i in $(S) ; do \
		sed "s/^CPU.*/CPU=\"$(host_cpu)\"/" < $(DESTDIR)$(bindir)/$$i > temp; \
		mv temp $(DESTDIR)$(bindir)/$$i; \
		chmod 755 $(DESTDIR)$(bindir)/$$i; \
	done
endif
