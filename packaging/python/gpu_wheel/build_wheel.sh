# Script to lunch Building of the GPU wheel
# We need to prepare the env and ensure neuron is cloned

module load nvhpc
unset CC CXX  # wheel extensions must not use nv pgi

cd /root
if [ ! -d nrn ]; then
    # TODO: Using the branch while not merged. Change to master later
    git clone https://github.com/neuronsimulator/nrn.git --depth=1 --branch=epic/gpu_wheel
fi

cd nrn

# TODO drop 38 for it to build on every Python version
bash packaging/python/build_wheels.bash linux 38
