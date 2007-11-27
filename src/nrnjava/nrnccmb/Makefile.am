javaSrc = Channels.java NrnKSChan.java

JAVAC = @JAVAC@

EXTRA_DIST = $(javaSrc)

# Only attempt to compile if ccmb.jar is available
# For normal installations this is not necessary since
# nrnccmb.jar is distributed with these classes precompiled
# and ccmb.jar will be
# installed when the channel builder is first invoked.
# Since Robert Cannon has a login on my machine he is able
# to deposit a new ccmb.jar file in the following location

#CCMBPATH = /home/rcc/ccmb.jar
CCMBPATH = $(HOME)/ccmb.jar

all: ../nrnccmb.jar

../nrnccmb.jar: $(javaSrc)
	( cd $(srcdir) ; \
	if test -r "$(CCMBPATH)" ; then \
	$(JAVAC) -classpath ..:$(CCMBPATH) $(javaSrc) ; \
	( cd .. ; jar cf nrnccmb.jar nrnccmb/*.class ) ; \
	fi \
	)
