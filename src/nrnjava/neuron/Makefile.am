javaSrc = NrnClassLoader.java\
	Neuron.java HocObject.java HocVariable.java HocVector.java \
	PWMListener.java Redirect.java

EXTRA_DIST = $(javaSrc)

JAVAC = @JAVAC@

all:
	( cd $(srcdir) ; \
	$(JAVAC) -classpath .:../nrnclsld.jar $(javaSrc) ; \
	)

