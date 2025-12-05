#!/usr/bin/env bash

# run the standard (non-GPU) container

set -eu


root_dir="$(cd "$(dirname "$0")/../"; pwd -P)"

if command -v docker >& /dev/null; then
    container_engine=docker
elif command -v podman >& /dev/null; then
    container_engine=podman
else
    echo "ERROR: docker or podman not found, you need to install one of those first and make them available in PATH"
    exit 2
fi


${container_engine} run \
    -v cache_volume:/opt/cache:rw \
    -v "${root_dir}":/nrn \
    --security-opt=label=disable \
    --cap-add=SYS_PTRACE \
    --security-opt seccomp=unconfined \
    --rm -ti \
    localhost/neuron_container /bin/bash
