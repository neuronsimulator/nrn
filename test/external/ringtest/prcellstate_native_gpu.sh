#!/usr/bin/env bash
# CPU vs native-GPU prcellstate parity for ringtest (rdcellstate.py).
# Usage: ./prcellstate_native_gpu.sh GID TSTOP [0|1]
#   0 = no step-phase dumps (default)
#   1 = dump phases for the last step ending at TSTOP
set -ex

gid=$1
tstop=$2
phases=${3:-0}

checkpoint_args=()
case "$phases" in
0)
    ;;
1)
    checkpoint_args=(-prcellstate_checkpoint_t "$tstop")
    ;;
*)
    echo "usage: $0 GID TSTOP [0|1]" >&2
    echo "  0 = no step-phase prcellstate files (default)" >&2
    echo "  1 = dump phases for the last step ending at TSTOP" >&2
    exit 1
    ;;
esac

run() {
  ./x86_64/special -python ringtest.py $* \
     -prcellstate_gid "$gid" -tstop "$tstop" "${checkpoint_args[@]}"
}

run ""
run "-gpu-native"

python ~/models/82894/rdcellstate.py --ignore-unused \
  "${gid}_-t${tstop}.nrndat" "${gid}_-gpu-t${tstop}.nrndat"