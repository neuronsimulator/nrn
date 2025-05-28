# Copyright 2023 Blue Brain Project, EPFL.
# See the top-level LICENSE file for details.
#
# SPDX-License-Identifier: Apache-2.0

import re


def camel_case_to_underscore(name):
    """convert string from 'AaaBbbbCccDdd' -> 'Aaa_Bbbb_Ccc_Ddd'"""
    s1 = re.sub("(.)([A-Z][a-z]+)", r"\1_\2", name)
    typename = re.sub("([a-z0-9])([A-Z])", r"\1_\2", s1)
    return typename


def to_snake_case(name):
    """convert string from 'AaaBbbbCccDdd' -> 'aaa_bbbb_ccc_ddd'"""
    return camel_case_to_underscore(name).lower()
