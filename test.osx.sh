set -xe

## python3.5 on OSX required following
# curl https://bootstrap.pypa.io/get-pip.py -o get-pip.py
# python3.5 get-pip.py

for test_config in python3.8:wheelhouse/NEURON-7.8.11-cp38-cp38-macosx_10_9_x86_64.whl \
                   python3.7:wheelhouse/NEURON-7.8.11-cp37-cp37m-macosx_10_9_x86_64.whl \
                   python3.6:wheelhouse/NEURON-7.8.11-cp36-cp36m-macosx_10_9_x86_64.whl \
                   python3.5:wheelhouse/NEURON-7.8.11-cp35-cp35m-macosx_10_9_intel.whl;
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
