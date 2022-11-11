FROM bbpgitlab.epfl.ch:5050/hpc/spacktainerizer/ubuntu_22.04/builder:2022-11-11-compiler-mods AS builder
FROM bbpgitlab.epfl.ch:5050/hpc/spacktainerizer/ubuntu_22.04/runtime:2022-11-11-compiler-mods

# Triggers building the 'builder' image, otherwise it is optimized away
COPY --from=builder /etc/debian_version /etc/debian_version

