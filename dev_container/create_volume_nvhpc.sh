#!/usr/bin/env bash

# one time script for creating a docker volume that has the NVHPC SDK installed
# default volume name: neuron_volume_nvhpc

set -eu

if [ "$#" -lt 1 ]; then
    echo "Usage: $0 <path-to-NVHPC-tar.gz> [<volume-name>]"
    exit 1
fi

if command -v docker >& /dev/null; then
    container_engine=docker
elif command -v podman >& /dev/null; then
    container_engine=podman
else
    echo "ERROR: docker or podman not found, you need to install one of those first and make them available in PATH"
    exit 2
fi

NVHPC_TARBALL="$1"
VOLUME_NAME="${2:-neuron_volume_nvhpc}"

current_dir="$(cd "$(dirname "$0")"; pwd -P)"

# Unique image tag and container name
IMAGE_TAG="nvhpc-installer-$(date +%s%N)"
TMP_CNAME="nvhpc-install-$(date +%s%N)"

# Build installer image
${container_engine} build -t "${IMAGE_TAG}" -f "${current_dir}/Dockerfile_nvhpc" "${current_dir}"

# Create image volume if it doesn't exist
${container_engine} volume inspect "${VOLUME_NAME}" >/dev/null 2>&1 || ${container_engine} volume create "${VOLUME_NAME}"

# One-shot installation
${container_engine} run --name "${TMP_CNAME}" --rm \
    -v "${VOLUME_NAME}:/opt/nvidia" \
    -v "${NVHPC_TARBALL}:/tmp/nvhpc.tar.gz:ro" \
    "${IMAGE_TAG}"

# Cleanup: remove image after use
${container_engine} rmi "${IMAGE_TAG}" >/dev/null 2>&1 || true

echo "Created ${container_engine} volume: ${VOLUME_NAME}"
