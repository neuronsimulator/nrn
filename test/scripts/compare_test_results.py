#!/usr/bin/env python3
"""Script to compare output of [Core]NEURON runs.

This utility is used to compare the results of different NEURON and CoreNEURON
runs and determine whether or not they are consistent with one another.
Positional arguments are of the form testname::type1::path1[::type2::path2]...

  Typical usage example:

  compare_test_results.py test1::asciispikes::path1 test2::asciispikes::path2
"""
import collections
import difflib
import glob
import itertools
import string
import sys


def parse_arguments(argv):
    parsed_data = collections.defaultdict(dict)

    def pairwise(iterable):
        "pairwise('ABCD') --> AB CD"
        return itertools.zip_longest(*[iter(iterable)] * 2)

    for test_spec in argv:
        test_spec_components = test_spec.split("::")
        if len(test_spec_components) % 2 != 1:
            raise Exception("Malformed test specification: " + test_spec)
        test_name = test_spec_components[0]
        for data_type, data_path in pairwise(test_spec_components[1:]):
            if test_name in parsed_data[data_type]:
                raise Exception(
                    "Got duplicate entry for data type {} in test {}".format(
                        data_type, test_name
                    )
                )
            parsed_data[data_type][test_name] = data_path
    return parsed_data


def load_ascii_spikes(data_pattern):
    spike_data = []
    for data_path in sorted(glob.glob(data_pattern)):
        with open(data_path, "r") as data_file:
            for line in data_file:
                time, gid = line.strip().split(maxsplit=1)
                spike_data.append((float(time), int(gid)))
    return spike_data


def check_compatibility(data_type, test_data):
    # TODO: support more data types, compare HDF5 files etc.
    if data_type != "asciispikes":
        raise Exception("Only {} test output is supported".format(data_type))
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
    names = sorted([name for name, _ in sorted_data])
    matrix = [[None] * len(names) for _ in range(len(names))]
    for (name1, data1), (name2, data2) in itertools.combinations(sorted_data, 2):
        result = data1 == data2  # TODO fuzzier comparison?
        name1_index, name2_index = names.index(name1), names.index(name2)
        min_index, max_index = (
            min(name1_index, name2_index),
            max(name1_index, name2_index),
        )
        matrix[min_index][max_index] = result
        print(" ".join([name1, "matches" if result else "DOES NOT match", name2]))
        if not result:
            # Try and print a helpful diff, this is a bit inelegant but it uses standard library...
            def as_strings(data):
                return [str(x) for x in data]

            for line in difflib.unified_diff(
                as_strings(data1),
                as_strings(data2),
                fromfile=name1,
                tofile=name2,
                lineterm="",
            ):
                print(line)
        results.append(result)
    print(" ".join(string.ascii_uppercase[i] for i in range(len(names))))
    print("-" * 2 * len(names))

    def val(row, col):
        if col < row:
            return " "
        return {None: ".", True: "M", False: "x"}[matrix[row][col]]

    for i in range(len(names)):
        print(
            " ".join(val(i, j) for j in range(len(names)))
            + " | "
            + string.ascii_uppercase[i]
            + " = "
            + names[i]
        )
    if any(results):
        print("M = match")
    if not all(results):
        print("x = mismatch")
    return all(results)


if __name__ == "__main__":
    try:
        test_specs = parse_arguments(sys.argv)
    except Exception as e:
        print(__doc__)
        print("ERROR parsing arguments: " + str(e))
        sys.exit(1)
    results = [
        check_compatibility(data_type=data_type, test_data=test_data)
        for data_type, test_data in test_specs.items()
    ]
    sys.exit(not all(results))
