#!/usr/bin/bash

sed_apply() (
    f=$1
    sedexp=$2
    echo "PATCHING $f with '$sedexp'"
    (cd $(dirname $f) && git checkout "$(basename $f)") && sed -i "$sedexp" "$f"
    grep 'version(' "$f"
)

patch_neuron() (
    if [ "$NEURON_BRANCH" ]; then
        pkg_file="${SPACK_ROOT}/var/spack/repos/builtin/packages/neuron/package.py"
        sedexp="/version.*tag=/d"  # Drop tags
        sedexp="$sedexp; /version.*preferred=/d"  # Drop preferred version
        sedexp="$sedexp; s#branch=[^)]*)#branch='$NEURON_BRANCH', preferred=True)#g"  # replace branch
        sed_apply "$pkg_file" "$sedexp"
    fi
)

set -e
set -x
source ${JENKINS_DIR:-.}/_env_setup.sh

# Install reportinglib & libsonata with spack to run reporting tests
spack install reportinglib%intel
spack install libsonata-report%intel

patch_neuron
spack install neuron+debug@develop~legacy-unit
source $SPACK_ROOT/share/spack/setup-env.sh
module av neuron reportinglib

