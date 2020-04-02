set -x
set -e

### FOR BB5
export PATH=/opt/rh/rh-python36/root/bin/:$PATH
export PATH=/gpfs/bbp.cscs.ch/ssd/apps/hpc/jenkins/deploy/external-libraries/2020-02-01/linux-rhel7-x86_64/gcc-8.3.0/python-3.7.4-tfxecymrkn/bin:$PATH

for test_config in python3.8:wheelhouse/NEURON-7.8.11-cp38-cp38-manylinux1_x86_64.whl \
                   python3.7:wheelhouse/NEURON-7.8.11-cp37-cp37m-manylinux1_x86_64.whl \
                   python3.6:wheelhouse/NEURON-7.8.11-cp36-cp36m-manylinux1_x86_64.whl \
                   python3.5:wheelhouse/NEURON-7.8.11-cp35-cp35m-manylinux1_x86_64.whl;
do
    python_wheel=${test_config#*:}
    python_exe=${test_config%:*}
    if [ -x "$(command -v ${python_exe})" ]; then
        echo "TESTING : $python_exe $python_wheel"
        ./test_wheel.sh $python_exe $python_wheel
    else
        echo "SKIPPING $python_exe!"
    fi
done

echo "--DONE--"
