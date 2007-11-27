#!/bin/sh

set -v
cd ..

#all the .o files
find . -name \*.o -print | sed '
/\/modlunit\//d
/\/nmodl\//d
/\/e_editor\//d
/\/ivoc\/classreg\.o/d
/\/ivoc\/datapath\.o/d
/\/ivoc\/nrnmain\.o/d
/\/ivoc\/ocjump\.o/d
/\/ivoc\/symdir\.o/d
/\/ivoc\/\.libs\/ivocman1\.o/d
/\/nrnoc\/cprop\.o/d
/\/oc\/\.libs\/code\.o/d
/\/oc\/\.libs\/hoc_init\.o/d
/\/oc\/\.libs\/hoc_oop\.o/d
/\/oc\/\.libs\/hocusr\.o/d
/\/oc\/\.libs\/plt\.o/d
/\/oc\/\.libs\/settext\.o/d
/\/oc\/\.libs\/spinit\.o/d
/\/oc\/\.libs\/spinit1\.o/d
/\/oc\/\.libs\/spinit2\.o/d
/\/oc\/\.libs\/modlreg\.o/d
/\/oc\/modlreg\.o/d
/\/memacs\/\.libs\/termio\.o/d
/\/memacs\/main\.o/d
/\/nvkludge\.o/d
/\/nocable\.o/d
/\/nrnnoiv\.o/d
/\/ockludge\.o/d
/\/ocnoiv\.o/d
/\/ocmain\.o/d
' > temp
#sed 's,^.*/,,' temp |sort|uniq -d
obj=`cat temp`

echo ${IVLIBDIR}
echo ${CFLAGS}

if test "$CFLAGS" != "-mno-cygwin" ; then

g++ -shared -nostdlib $obj \
  -L${IVLIBDIR} -lIVhines \
  -L/usr/lib/gcc/i686-pc-cygwin/3.4.4 -lstdc++ -lgcc \
  -lcygwin -luser32 -lkernel32 -ladvapi32 -lshell32 \
  $LIBS \
  -L/usr/bin -lpython2.5 \
  -lgdi32 -lcomdlg32 -lncurses -lm \
  -o mswin/hocmodule.dll \
  -Wl,--enable-auto-image-base \
  -Xlinker --out-implib -Xlinker mswin/libhocmodule.dll.a

cd mswin
g++ -g -O2 -mwindows -e _mainCRTStartup -o nrniv.exe \
  ../ivoc/nrnmain.o ../oc/modlreg.o \
  -L. -lhocmodule \
  -lncurses \
  -L${IVLIBDIR} -lIVhines \
  -lgdi32 -lcomdlg32 \
  -L/usr/lib/python2.5/config -lpython2.5 -ldl

else

g++ -shared -mno-cygwin $obj \
  $LIBS \
  -L/cygdrive/c/Python24/libs -lpython24 \
  -o mswin/hocmodule.dll \
  -Wl,--enable-auto-image-base \
  -Xlinker --out-implib -Xlinker mswin/libhocmodule.dll.a

cd mswin
g++ -g -O2 -mno-cygwin -e _mainCRTStartup -o nrniv.exe \
  ../ivoc/nrnmain.o ../oc/modlreg.o \
  -L. -lhocmodule \
  -L/cygdrive/c/Python24/libs -lpython24

fi

#mv nrniv.exe c:/nrn61/bin
#cd ..
#mv hocmodule.dll c:/nrn61/bin

