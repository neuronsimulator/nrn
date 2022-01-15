# Upstream modules
unset MODULEPATH
source /gpfs/bbp.cscs.ch/apps/bsd/config/modules.sh
module load unstable

# Local spack
BUILD_HOME="${WORKSPACE}/BUILD_HOME"
INSTALL_HOME="${WORKSPACE}/INSTALL_HOME"
export SPACK_ROOT="${BUILD_HOME}/spack"
export SPACK_SYSTEM_CONFIG_PATH=/gpfs/bbp.cscs.ch/ssd/apps/bsd/config
export SPACK_USER_CACHE_PATH="${SPACK_INSTALL_PREFIX:-${INSTALL_HOME}}"
export PATH=/usr/bin:$PATH

if [ -d "$SPACK_ROOT" ]; then
    source $SPACK_ROOT/share/spack/setup-env.sh
fi

# Common init
unset $(env|awk -F= '/^(PMI|SLURM)_/ {if ($1 != "SLURM_ACCOUNT") print $1}')
module load hpe-mpi
