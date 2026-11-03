#!/usr/bin/env python3
"""
Barebones utility for changing the name of the package in `pyproject.toml`.
"""
import re
from argparse import ArgumentParser
from pathlib import Path

import tomli
import tomli_w


def main():
    parser = ArgumentParser(
        description="Script for changing the name of a project in a pyproject.toml file",
    )
    parser.add_argument("pyproject_file", help="The path to the pyproject.toml file")
    parser.add_argument("name", help="The new name of the project")
    args = parser.parse_args()

    if not Path(args.pyproject_file).exists():
        raise FileNotFoundError(f"File {args.pyproject_file} not found")

    with open(args.pyproject_file, "rb") as f:
        toml_dict = tomli.load(f)

    # check name is conforms to the naming convention
    # see:
    # https://packaging.python.org/en/latest/specifications/name-normalization/

    if not re.match(
        r"^([A-Z0-9]|[A-Z0-9][A-Z0-9._-]*[A-Z0-9])$",
        args.name,
        flags=re.IGNORECASE,
    ):
        raise ValueError(
            f"The package name {args.name} does not conform "
            "to the standard Python package naming convention"
        )

    toml_dict["project"]["name"] = args.name

    with open(args.pyproject_file, "wb") as f:
        tomli_w.dump(toml_dict, f)


if __name__ == "__main__":
    main()
