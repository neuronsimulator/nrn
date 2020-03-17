# Upstream modules
unset MODULEPATH
source /gpfs/bbp.cscs.ch/apps/hpc/jenkins/config/modules.sh
module load unstable

# Local spack
BUILD_HOME="${WORKSPACE}/BUILD_HOME"
INSTALL_HOME="${WORKSPACE}/INSTALL_HOME"
export SPACK_ROOT="${BUILD_HOME}/spack"
export SPACK_INSTALL_PREFIX="${SPACK_INSTALL_PREFIX:-${INSTALL_HOME}}"
export SOFTS_DIR_PATH=$SPACK_INSTALL_PREFIX  # Deprecated, but might still be reqd
export PATH=/usr/bin:$PATH

if [ -d "$SPACK_ROOT" ]; then
    source $SPACK_ROOT/share/spack/setup-env.sh
fi

# Common init
unset $(env|awk -F= '/^(PMI|SLURM)_/ {if ($1 != "SLURM_ACCOUNT") print $1}')
module load hpe-mpi
