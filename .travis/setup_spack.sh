#!/bin/bash
set -xe


# spack repository
cd $TRAVIS_BUILD_DIR
git clone --depth 1 https://github.com/pramodskumbhar/spack.git -b stable


# enables spack command line support
set +x
source $SPACK_ROOT/share/spack/setup-env.sh
set -x


# add spack packages repository
cd $TRAVIS_BUILD_DIR
git clone --depth 1 https://github.com/pramodskumbhar/spack-packages.git -b stable
set +x
spack repo add --scope site ./spack-packages


# copy os specific spack configuration
cd $TRAVIS_BUILD_DIR
mkdir -p $HOME/.spack/
cp .travis/packages.$TRAVIS_OS_NAME.yaml $HOME/.spack/packages.yaml
cp .travis/modules.yaml $HOME/.spack/


# external packages are already installed on system, this install step just register them to spack
spack install flex bison automake autoconf libtool pkg-config ncurses mpich openmpi python@3 python@2.7


# find python2/3 to avoid hardcoded paths in packages.yaml
PYTHON_2_PATH=`python -c "from distutils.sysconfig import get_config_var; print(get_config_var('prefix'))"`
PYTHON_3_PATH=`python3 -c "from distutils.sysconfig import get_config_var; print(get_config_var('prefix'))"`


# setup python path (gnu vs osx sed)
if [ "$TRAVIS_OS_NAME" == "linux" ]; then
    sed -i "s#PYTHON_2_PATH#$PYTHON_2_PATH#g" $HOME/.spack/packages.yaml
    sed -i "s#PYTHON_3_PATH#$PYTHON_3_PATH#g" $HOME/.spack/packages.yaml
else
    sed -i '' "s#PYTHON_2_PATH#$PYTHON_2_PATH#g" $HOME/.spack/packages.yaml
    sed -i '' "s#PYTHON_3_PATH#$PYTHON_3_PATH#g" $HOME/.spack/packages.yaml
fi
