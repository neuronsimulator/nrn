# install rpms
set -euo pipefail

mkdir -p rpms
pushd rpms

wget --no-verbose \
  https://developer.download.nvidia.com/hpc-sdk/21.7/nvhpc-2021-21.7-1.x86_64.rpm \
  https://developer.download.nvidia.com/hpc-sdk/21.7/nvhpc-21-7-21.7-1.x86_64.rpm
  # https://developer.download.nvidia.com/hpc-sdk/21.7/nvhpc-21-7-cuda-multi-21.7-1.x86_64.rpm

yum install -y *.rpm

popd

rm -rf rpms
