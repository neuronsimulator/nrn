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

set -x
set -e

export SPACK_ROOT="$WORKSPACE/BUILD_HOME/spack/"
export SPACK_INSTALL_PREFIX="${SPACK_INSTALL_PREFIX:-${WORKSPACE}/INSTALL_HOME}"
source ${SPACK_ROOT}/share/spack/setup-env.sh
source /gpfs/bbp.cscs.ch/apps/hpc/jenkins/config/modules.sh
export PATH=$WORKSPACE/BUILD_HOME/spack/bin:/usr/bin:$PATH
export MODULEPATH=$SPACK_INSTALL_PREFIX/modules/tcl/$(spack arch):$MODULEPATH

unset $(env|awk -F= '/^(PMI|SLURM)_/ {if ($1 != "SLURM_ACCOUNT") print $1}')

patch_neuron

spack install neuron+debug@develop
module av neuron
