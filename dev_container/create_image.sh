#!/usr/bin/env bash

# create the base Podman/Docker image
# default image name: neuron_container

set -eu

current_dir="$(cd "$(dirname "$0")"; pwd -P)"

if command -v docker >& /dev/null; then
    container_engine=docker
elif command -v podman >& /dev/null; then
    container_engine=podman
else
    echo "ERROR: docker or podman not found, you need to install one of those first and make them available in PATH"
    exit 2
fi

IMAGE_NAME="${1:-localhost/neuron_container}"

${container_engine} build -t "${IMAGE_NAME}" -f "${current_dir}/Dockerfile" "${current_dir}"

echo "Created ${container_engine} image: ${IMAGE_NAME}"
