#build_wheel.sh

module load nvhpc
unset CC CXX  # wheel extensions must not use nv pgi

git clone https://github.com/neuronsimulator/nrn.git --depth=1 --branch=epic/gpu_wheel

cd nrn

bash packaging/python/build_wheels.bash linux 38
