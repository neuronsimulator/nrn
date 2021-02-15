#!/usr/bin/env python3
"""Script to compare output of [Core]NEURON runs.

This utility is used to compare the results of different NEURON and CoreNEURON
runs and determine whether or not they are consistent with one another.
Positional arguments are of the form testname::type1::path1[::type2::path2]...

  Typical usage example:

  compare_test_results.py test1::asciispikes::path1 test2::asciispikes::path2
"""
import collections
import itertools
import subprocess
import sys
from pprint import pprint


def parse_arguments(argv):
    parsed_data = collections.defaultdict(dict)

    def pairwise(iterable):
        "pairwise('ABCD') --> AB CD"
        return itertools.zip_longest(*[iter(iterable)] * 2)

    for test_spec in argv:
        test_spec_components = test_spec.split('::')
        if len(test_spec_components) % 2 != 1:
            raise Exception('Malformed test specification: ' + test_spec)
        test_name = test_spec_components[0]
        for data_type, data_path in pairwise(test_spec_components[1:]):
            if test_name in parsed_data[data_type]:
                raise Exception(
                    'Got duplicate entry for data type {} in test {}'.format(
                        data_type, test_name))
            parsed_data[data_type][test_name] = data_path
    return parsed_data


def load_ascii_spikes(data_path):
    spike_data = []
    with open(data_path, 'r') as data_file:
        for line in data_file:
            time, gid = line.strip().split(maxsplit=1)
            spike_data.append((float(time), int(gid)))
    return spike_data


def check_compatibility(data_type, test_data):
    # TODO: support more data types, compare HDF5 files etc.
    if data_type != 'asciispikes':
        raise Exception('Only {} test output is supported'.format(data_type))
    # Load the ASCII output files
    sorted_data = []
    for test_name, data_path in test_data.items():
        spike_data = load_ascii_spikes(data_path)
        spike_data.sort()  # sort by time, then by ID
        sorted_data.append((test_name, spike_data))
    # Clearly this is a bit excessive because the relationship is transitive,
    # but hopefully the output will be helpful when finding the source of a
    # failure.
    results = []
    for (name1, data1), (name2,
                         data2) in itertools.combinations(sorted_data, 2):
        result = (data1 == data2)  # TODO fuzzier comparison?
        print(' '.join(
            [name1, 'matches' if result else 'DOES NOT match', name2]))
        results.append(result)
    return all(results)


if __name__ == '__main__':
    try:
        test_specs = parse_arguments(sys.argv)
    except Exception as e:
        print(__doc__)
        print('ERROR parsing arguments: ' + str(e))
        sys.exit(1)
    results = [
        check_compatibility(data_type=data_type, test_data=test_data)
        for data_type, test_data in test_specs.items()
    ]
    sys.exit(not all(results))
