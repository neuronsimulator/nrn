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
from pathlib import Path, PurePath
import shutil
import stat
import subprocess
import tempfile

import jinja2

from parser import LanguageParser
import node_info
import utils


logging.basicConfig(level=logging.INFO, format='%(message)s')
parser = argparse.ArgumentParser()
parser.add_argument('--clang-format', help="Path to clang-format executable")
parser.add_argument('--clang-format-opts', help="clang-format options", nargs='+')
parser.add_argument('--base-dir')

args = parser.parse_args()
clang_format = args.clang_format
if clang_format:
    clang_format = [clang_format]
    if args.clang_format_opts:
        clang_format += args.clang_format_opts

# parse NMODL and codegen definition files to get AST nodes
nmodl_nodes = LanguageParser("nmodl.yaml").parse_file()
codegen_nodes = LanguageParser("codegen.yaml").parse_file()

# combine NMODL and codegen nodes for whole AST
nodes = nmodl_nodes
nodes.extend(x for x in codegen_nodes if x not in nodes)

# directory containing all templates
templates_dir = Path(__file__).parent / 'templates'

# destination directory to render templates
destination_dir = Path(args.base_dir) or Path(__file__).resolve().parent.parent

# templates will be created and clang-formated in tempfile.
# create temp directory and copy .clang-format file for correct formating
clang_format_file = Path(Path(__file__).resolve().parent.parent.parent / ".clang-format")
temp_dir = tempfile.mkdtemp()
os.chdir(temp_dir)

if clang_format_file.exists():
    shutil.copy2(clang_format_file, temp_dir)

env = jinja2.Environment(loader=jinja2.FileSystemLoader(str(templates_dir)),
                         trim_blocks=True,
                         lstrip_blocks=True)
env.filters['snake_case'] = utils.to_snake_case

updated_files = []

for path in templates_dir.iterdir():
    sub_dir = PurePath(path).name
    (destination_dir / sub_dir).mkdir(parents=True, exist_ok=True)
    for filepath in path.glob('*.[ch]pp'):
        source_file = os.path.join(sub_dir, filepath.name)
        destination_file = destination_dir / sub_dir / filepath.name
        template = env.get_template(source_file)
        content = template.render(nodes=nodes, node_info=node_info)
        if destination_file.exists():
            # render template in temporary file and update target file
            # ONLY if different (to save a lot of build time)
            fd, tmp_path = tempfile.mkstemp(dir=temp_dir)
            os.write(fd, content.encode('utf-8'))
            os.close(fd)
            if clang_format:
                subprocess.check_call(clang_format + ['-i', tmp_path])
            if not filecmp.cmp(str(destination_file), tmp_path):
                # ensure destination file has write permissions
                mode = os.stat(destination_file).st_mode
                rm_write_mask = stat.S_IWUSR | stat.S_IWGRP | stat.S_IWOTH
                os.chmod(destination_file, mode | rm_write_mask)
                shutil.move(tmp_path, destination_file)
                updated_files.append(str(filepath.name))
        else:
            with destination_file.open('w') as fd:
                fd.write(content)
                updated_files.append(str(filepath.name))
            if clang_format:
                subprocess.check_call(clang_format + ['-i', destination_file])
        # remove write permissions on the generated file
        mode = os.stat(destination_file).st_mode
        rm_write_mask = ~ (stat.S_IWUSR | stat.S_IWGRP | stat.S_IWOTH)
        os.chmod(destination_file, mode & rm_write_mask)


if updated_files:
    logging.info('       Updating out of date template files : %s', ' '.join(updated_files))

# remove temp directory
shutil.rmtree(temp_dir)
