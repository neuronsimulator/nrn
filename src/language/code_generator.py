# ***********************************************************************
# Copyright (C) 2018-2019 Blue Brain Project
#
# This file is part of NMODL distributed under the terms of the GNU
# Lesser General Public License. See top-level LICENSE file for details.
# ***********************************************************************

import argparse
import filecmp
import logging
import os
from pathlib import Path
import shutil
import subprocess
import tempfile

import jinja2

from parser import LanguageParser
import node_info
import utils


logging.basicConfig(level=logging.INFO, format='%(message)s')
parser = argparse.ArgumentParser()
parser.add_argument('--clang-format', help="Path to clang-format executable")
parser.add_argument('--clang-format-opts', help="Path to clang-format executable", nargs='+')
parser.add_argument('--base-dir')
args = parser.parse_args()
clang_format = args.clang_format
if clang_format:
    clang_format = [clang_format]
    if args.clang_format_opts:
        clang_format += args.clang_format_opts

# parse nmodl definition file and get list of abstract nodes
nmodl_nodes = LanguageParser("nmodl.yaml").parse_file()
codegen_nodes = LanguageParser("codegen.yaml").parse_file()

# combine ast nodes from NMODL specification and codegen specification
nodes = nmodl_nodes
nodes.extend(x for x in codegen_nodes if x not in nodes)

templates = Path(__file__).parent / 'templates'

basedir = Path(args.base_dir) or Path(__file__).resolve().parent.parent
for subdir in ['pybind', 'visitors', 'ast']:
    (basedir / subdir).mkdir(parents=True, exist_ok=True)

env = jinja2.Environment(loader=jinja2.FileSystemLoader(str(templates)),
                         trim_blocks=True,
                         lstrip_blocks=True)
env.filters['snake_case'] = utils.to_snake_case

updated_files = []

for fn in templates.glob('*.[ch]pp'):
    if fn.name.startswith('py'):
        filename = basedir / 'pybind' / fn.name
    elif 'visitor' in fn.name:
        filename = basedir / 'visitors' / fn.name
    else:
        filename = basedir / 'ast' / fn.name
    template = env.get_template(fn.name)
    content = template.render(nodes=nodes, node_info=node_info)
    if filename.exists():
        # render template in temporary file
        # and update target file ONLY if different
        # to save a lot of build time
        fd, tmp_path = tempfile.mkstemp()
        os.write(fd, content.encode('utf-8'))
        os.close(fd)
        if clang_format:
            subprocess.check_call(clang_format + ['-i', tmp_path])
        if not filecmp.cmp(str(filename), tmp_path):
            shutil.move(tmp_path, filename)
            updated_files.append(str(fn.name))
    else:
        with filename.open('w') as fd:
            fd.write(content)
            updated_files.append(str(fn.name))
        if clang_format:
            subprocess.check_call(clang_format + ['-i', filename])

if updated_files:
    logging.info('       Updating out of date template files : %s', ' '.join(updated_files))
