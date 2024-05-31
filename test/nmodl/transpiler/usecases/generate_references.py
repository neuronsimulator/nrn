#!/usr/bin/env python3
import sys
import glob
import os
import json
import subprocess


def substitute_line(lines, starts_with, replacement):
    for k, line in enumerate(lines):
        if line.startswith(starts_with):
            lines[k] = replacement
            break


def generate_references(nmodl, usecase_dir, output_dir, nmodl_flags):
    mod_files = glob.glob(os.path.join(usecase_dir, "*.mod"))
    for mod_file in mod_files:
        try:
            subprocess.run(
                ["nmodl", mod_file, *nmodl_flags, "-o", output_dir], check=True
            )
        except subprocess.CalledProcessError as e:
            print(e.output)
            raise e

    cxx_files = glob.glob(os.path.join(output_dir, "*.cpp"))

    for cxx_file in cxx_files:
        with open(cxx_file, "r") as f:
            cxx = f.readlines()

        sub_date = "Created         : ", "Created         : DATE\n"
        substitute_line(cxx, *sub_date)

        sub_version = "NMODL Compiler  : ", "NMODL Compiler  : VERSION\n"
        substitute_line(cxx, *sub_version)

        with open(cxx_file, "w") as f:
            f.writelines(cxx)


def load_config(script_dir, usecase_dir):
    config = os.path.join(usecase_dir, "references.json")
    if os.path.exists(config):
        with open(config, "r") as f:
            return json.load(f)
    else:
        return [
            {"output_reldir": "coreneuron", "nmodl_flags": []},
            {"output_reldir": "neuron", "nmodl_flags": ["--neuron"]},
        ]


if __name__ == "__main__":
    if len(sys.argv) != 4:
        print(f"Usage: {sys.argv[0]} NMODL USECASE_DIR OUTPUT_DIR")
        sys.exit(-1)

    nmodl = sys.argv[1]
    usecase_dir = sys.argv[2]
    output_basedir = sys.argv[3]
    script_dir = os.path.dirname(sys.argv[0])

    config = load_config(script_dir=script_dir, usecase_dir=usecase_dir)
    for c in config:
        output_dir = os.path.join(output_basedir, c["output_reldir"])
        generate_references(nmodl, usecase_dir, output_dir, c["nmodl_flags"])
