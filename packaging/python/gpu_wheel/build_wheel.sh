#build_wheel.sh

module load nvhpc

git clone https://github.com/neuronsimulator/nrn.git --depth=1

cd nrn

bash packaging/python/build_wheels.bash linux 38
