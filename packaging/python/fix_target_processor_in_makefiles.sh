#!/usr/bin/env bash
set -ex

instdir="$1"

# We build the GPU wheels with -tp=haswell for portability, but we don't want to
# embed this in the Makefiles in the wheels themselves to (hopefully) give
# better performance when users run nrnivmodl themselves.
for makefile in bin/nrnmech_makefile share/coreneuron/nrnivmodl_core_makefile
do
  sed -i.old -e 's#-tp=haswell##g' "${instdir}/${makefile}"
  ! diff -u "${instdir}/${makefile}.old" "${instdir}/${makefile}"
  rm "${instdir}/${makefile}.old"
done