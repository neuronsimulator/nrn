#!/bin/sh
# ------------------------------------------------------------
# Script for RTD build(used for notebooks execution & doxygen)
# ------------------------------------------------------------
set -e

BUILD_DIR=../build_rtd

rm -rf $BUILD_DIR
mkdir  $BUILD_DIR
cd $BUILD_DIR

cmake -DNRN_ENABLE_MPI=OFF -DNRN_ENABLE_INTERVIEWS=OFF ..
make notebooks
make doxygen
