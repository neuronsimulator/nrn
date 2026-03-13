#!/usr/bin/env python3
import re
import sys


def is_pep440_canonical_version(version):
    # Regex matching your simplified version (no epoch, as per your latest attempt)
    regex = r"^(0|[1-9][0-9]*)(\.(0|[1-9][0-9]*))*((a|b|rc)(0|[1-9][0-9]*))?(\.post(0|[1-9][0-9]*))?(\.dev(0|[1-9][0-9]*))?$"
    return bool(re.match(regex, version))


if __name__ == "__main__":
    if len(sys.argv) != 2:
        print("Usage: check_pep440_version.py <version>", file=sys.stderr)
        sys.exit(1)
    version = sys.argv[1]
    result = is_pep440_canonical_version(version)
    print("TRUE" if result else "FALSE")
    sys.exit(0)
