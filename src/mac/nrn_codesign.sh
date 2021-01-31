#!/bin/bash

# All binaries that will be part of the package must be signed with a
# Developer ID Application certificate in order to be notarized.
# The information in this file is not private since the installer may obtain
# signing info with, eg
# codesign --verify --verbose $inst/bin/nrniv
# codesign --display --verbose=4 $inst/bin/nrniv
# codesign --display --entitlements :- $inst/bin/nrniv

# https://forum.xojo.com/t/how-to-add-entitlements-to-a-xojo-app-using-codesign/49735/9
# was of help with respect to entitlements.
# https://www.davidebarranca.com/2019/04/notarizing-installers-for-macos-catalina/
# was extremely helpful with respect to notarization (of which this signing
# is a prerequisite).

# "$1" The install folder
inst="$1"
# "$2" Path to folder containing nrncodesign.sh
cursrc="$2"
entitlements_path="$cursrc/nrn_entitlements.plist"

identity="Developer ID Application: Michael Hines (8APKV34642)"

apps="idraw mknrndll modlunit mos2nrn neurondemo nrngui"

executables="idraw modlunit nocmodl nrniv"

libraries="
  libivx11dynam.dylib libnrniv.dylib libnrnmpi_ompi.dylib
  libnrnpython2.dylib libnrnpython3.dylib librxdmath.dylib
"

pylibraries="
  hoc.cpython-36m-darwin.so
  hoc.cpython-39-darwin.so
  hoc.cpython-37m-darwin.so
  hoc.cpython-38-darwin.so
  hoc.so
  rxd/geometry3d/surfaces.cpython-39-darwin.so
  rxd/geometry3d/ctng.cpython-39-darwin.so
  rxd/geometry3d/ctng.so
  rxd/geometry3d/ctng.cpython-37m-darwin.so
  rxd/geometry3d/surfaces.cpython-36m-darwin.so
  rxd/geometry3d/graphicsPrimitives.cpython-36m-darwin.so
  rxd/geometry3d/surfaces.cpython-38-darwin.so
  rxd/geometry3d/ctng.cpython-38-darwin.so
  rxd/geometry3d/ctng.cpython-36m-darwin.so
  rxd/geometry3d/graphicsPrimitives.cpython-39-darwin.so
  rxd/geometry3d/surfaces.cpython-37m-darwin.so
  rxd/geometry3d/surfaces.so
  rxd/geometry3d/graphicsPrimitives.cpython-38-darwin.so
  rxd/geometry3d/graphicsPrimitives.so
  rxd/geometry3d/graphicsPrimitives.cpython-37m-darwin.so
"

demolibnrnmech="x86_64/libnrnmech.dylib x86_64/.libs/libnrnmech.so"

#codesign a file, $1, prefixed by $inst
sign() {
  # I don't think all these options are needed for all cases, but ...
  # --options runtime : Seems to resolve the notarization issue of
  #                     "Enable hardened runtime"
  # --deep : Not sure what it does exactly. Below we separately do
  #          app and app/Contents/Resources/Droplet.icns            
  # --force : replaces old signature (if any) with this new one
  f="$inst"/"$1"
  # No error if file does not exist (some python or interviews may not exist)
  #   Note: An app is a folder to the shell.
  if test -f "$f" -o -d "$f" ; then
    echo "codesign \"$f\""
    codesign  --sign "$identity" \
      --options runtime \
      --deep \
      --force \
      --entitlements "$entitlements_path" \
      "$f"
  else
    echo "codesign: \"$f\" does not exist"
  fi
}

# Sign a bunch of files, "$2", prefixed by $1, which will be prefixed by $inst
sign_several() {
  for i in $2 ; do
    sign "$1"/"$i"
  done
}

# Signing an app, "$1", also involves signing the droplet.icns
sign_app() {
  # Droplet has to be signed first then the app
#  sign "$1"/Contents/Resources/droplet.icns
  sign "$1"
}

for i in $apps ; do
  sign_app "$i.app"
done

sign_several "bin" "$executables"

sign_several "lib" "$libraries"

sign_several "lib/python/neuron" "$pylibraries"

sign_several "share/nrn/demo/release" "$demolibnrnmech"
