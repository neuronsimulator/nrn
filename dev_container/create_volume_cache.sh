#!/usr/bin/env bash

# one time script for creating a docker volume that hosts the Python and compiler cache
# default volume name: neuron_volume_cache


set -eu

if command -v docker >& /dev/null; then
    container_engine=docker
elif command -v podman >& /dev/null; then
    container_engine=podman
else
    echo "ERROR: docker or podman not found, you need to install one of those first and make them available in PATH"
    exit 2
fi

VOLUME_NAME="${1:-neuron_volume_cache}"

# Create image volume if it doesn't exist
${container_engine} volume inspect "${VOLUME_NAME}" >/dev/null 2>&1 || ${container_engine} volume create "${VOLUME_NAME}"

echo "Created ${container_engine} volume: ${VOLUME_NAME}"
