# ***********************************************************************
# Copyright (C) 2018-2022 Blue Brain Project
#
# This file is part of NMODL distributed under the terms of the GNU
# Lesser General Public License. See top-level LICENSE file for details.
# ***********************************************************************

import argparse
import collections
import filecmp
import itertools
import logging
import math
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


LOGGING_FORMAT = "%(levelname)s:%(name)s: %(message)s"
LOGGER = logging.getLogger("NMODLCodeGen")


class CodeGenerator(
    collections.namedtuple(
        "Base",
        [
            "base_dir",
            "clang_format",
            "jinja_env",
            "jinja_templates_dir",
            "modification_date",
            "nodes",
            "py_files",
            "temp_dir",
            "this_dir",
            "yaml_files",
        ],
    )
):
    """Code generator application

    Attributes:
        base_dir: output root directory where Jinja templates are rendered
        clang_format: clang-format command line if C++ files have to be formatted, `None` otherwise
        py_files: list of Path objects to the Python files used by this program
        yaml_files: list of Path object to YAML files describing the NMODL language
        modification_date: most recent modification date of the Python and YAML files in this directory
        jinja_templates_dir: directory containing all Jinja templates
        jinja_env: Jinja Environment object
        this_dir: Path instance to the directory containing this file
        temp_dir: path to the directory where to create temporary files
    """

    def __new__(cls, base_dir, clang_format=None):
        this_dir = Path(__file__).parent.resolve()
        jinja_templates_dir = this_dir / "templates"
        py_files = [Path(p).relative_to(this_dir) for p in this_dir.glob("*.py")]
        yaml_files = [Path(p).relative_to(this_dir) for p in this_dir.glob("*.yaml")]
        self = super(CodeGenerator, cls).__new__(
            cls,
            base_dir=base_dir,
            clang_format=clang_format,
            this_dir=this_dir,
            jinja_templates_dir=jinja_templates_dir,
            jinja_env=jinja2.Environment(
                loader=jinja2.FileSystemLoader(str(jinja_templates_dir)),
                keep_trailing_newline=True,
                trim_blocks=True,
                lstrip_blocks=True,
            ),
            temp_dir=tempfile.mkdtemp(),
            py_files=py_files,
            yaml_files=yaml_files,
            modification_date=max(
                [os.path.getmtime(p) for p in itertools.chain(py_files, yaml_files)]
            ),
            nodes=LanguageParser(this_dir / "nmodl.yaml").parse_file(),
        )

        self.jinja_env.filters["snake_case"] = utils.to_snake_case

        # copy clang-format configuration file in the temporary directory is present
        # in the repository root.
        cf_config = Path(__file__).parent.parent.parent / ".clang-format"
        if cf_config.exists():
            shutil.copy2(cf_config, self.temp_dir)

        # combine NMODL and codegen nodes for whole AST
        codegen_nodes = LanguageParser(self.this_dir / "codegen.yaml").parse_file()
        self.nodes.extend(x for x in codegen_nodes if x not in self.nodes)

        return self

    def jinja_template(self, path):
        """Construct a Jinja template object

        Args:
            path: Path object of a file inside the Jinja template directory

        Returns:
            A jinja template object
        """
        name = str(path.relative_to(self.jinja_templates_dir))
        return self.jinja_env.get_template(name)

    def _cmake_deps_task(self, tasks):
        """"Construct the JinjaTask generating the CMake file exporting all dependencies

        Args:
            tasks: list of JinjaTask objects

        Returns:
            An instance of JinjaTask
        """
        input = self.jinja_templates_dir / "code_generator.cmake"
        output = self.this_dir / input.name
        inputs = set()
        outputs = dict()
        for task in tasks:
            inputs.add(task.input.relative_to(self.jinja_templates_dir))
            for dep in task.extradeps:
                inputs.add(dep.relative_to(self.jinja_templates_dir))
            dir, name = task.output.relative_to(self.base_dir).parts
            outputs.setdefault(dir, []).append(name)

        return JinjaTask(
            app=self,
            input=input,
            output=output,
            context=dict(
                templates=inputs,
                outputs=outputs,
                py_files=self.py_files,
                yaml_files=self.yaml_files,
            ),
            extradeps=None,
        )

    def workload(self):
        """Compute the list of Jinja tasks to perform

        Returns:
            A list of JinjaTask objects
        """
        # special template "ast/node.hpp used to generate multiple .hpp files
        node_hpp_tpl = self.jinja_templates_dir / "ast" / "node.hpp"
        pyast_cpp_tpl = self.jinja_templates_dir / "pybind" / "pyast.cpp"
        pynode_cpp_tpl = self.jinja_templates_dir / "pybind" / "pynode.cpp"
        # special template only included by other templates
        node_class_tpl = self.jinja_templates_dir / "ast" / "node_class.template"
        # Additional dependencies Path -> [Path, ...]
        extradeps = collections.defaultdict(list, {node_hpp_tpl: [node_class_tpl]})
        # Additional Jinja context set when rendering the template
        num_pybind_files = 2
        extracontext = collections.defaultdict(
            dict,
            {
                pyast_cpp_tpl: {
                    "setup_pybind_methods": [
                        "init_pybind_classes_{}".format(x)
                        for x in range(num_pybind_files)
                    ]
                }
            },
        )

        tasks = []
        for path in self.jinja_templates_dir.iterdir():
            sub_dir = PurePath(path).name
            # create output directory if missing
            (self.base_dir / sub_dir).mkdir(parents=True, exist_ok=True)
            for filepath in path.glob("*.[ch]pp"):
                if filepath == node_hpp_tpl:
                    # special treatment for this template.
                    # generate one C++ header per AST node type
                    for node in self.nodes:
                        task = JinjaTask(
                            app=self,
                            input=filepath,
                            output=self.base_dir / node.cpp_header,
                            context=dict(node=node, **extracontext[filepath]),
                            extradeps=extradeps[filepath],
                        )
                        tasks.append(task)
                        yield task
                elif filepath == pynode_cpp_tpl:
                    chunk_length = math.ceil(len(self.nodes) / num_pybind_files)
                    for chunk_k in range(num_pybind_files):
                        task = JinjaTask(
                            app=self,
                            input=filepath,
                            output=self.base_dir / sub_dir / "pynode_{}.cpp".format(chunk_k),
                            context=dict(
                                nodes=self.nodes[
                                    chunk_k * chunk_length : (chunk_k + 1) * chunk_length
                                ],
                                setup_pybind_method="init_pybind_classes_{}".format(chunk_k),
                            ),
                            extradeps=extradeps[filepath],
                        )
                        tasks.append(task)
                        yield task
                else:
                    task = JinjaTask(
                        app=self,
                        input=filepath,
                        output=self.base_dir / sub_dir / filepath.name,
                        context=dict(nodes=self.nodes, node_info=node_info, **extracontext[filepath]),
                        extradeps=extradeps[filepath],
                    )
                    tasks.append(task)
                    yield task
        yield self._cmake_deps_task(tasks)


class JinjaTask(
    collections.namedtuple(
        "JinjaTask", ["app", "input", "output", "context", "extradeps"]
    )
):
    """Generate a file with Jinja

    Attributes:
        app: CodeGenerator object
        input: Path to the template on the filesystem
        output: Path to the destination file to create
        context: dict containing the variables passed to Jinja renderer
    """

    def execute(self):
        """"Perform the Jinja task

        Execute Jinja renderer if the output file is out-of-date.

        Returns:
            True if the output file has been created or modified, False otherwise
        """
        if self.out_of_date:
            return self.render()
        return False

    @property
    def logger(self):
        return LOGGER.getChild(str(self.output))

    @property
    def out_of_date(self):
        """Check if the output file have to be generated

        Returns:
            True if the output file is missing or if one of its dependencies has changed:
            - the Jinja template
            - the python files used by this program
            - the YAML files describing the NMODL language
        """
        if not self.output.exists():
            self.logger.debug("output does not exist")
            return True
        output_mdate = max(os.path.getctime(self.output), os.path.getmtime(self.output))
        if self.app.modification_date > output_mdate:
            self.logger.debug("output is out-of-date")
            return True

        deps = self.extradeps or []
        deps.append(self.input)
        for dep in deps:
            if os.path.getmtime(dep) > output_mdate:
                return True
        self.logger.debug("output is up-to-date")

        return False

    def format_output(self, file):
        """Format a given file

        On applies to C++ files. Use ClangFormat if enabled

        Arguments:
            file: path to the file to format. filename might be temporary
            language: c++ or cmake

        """
        if self.language == "c++" and self.app.clang_format:
            LOGGER.debug("Formatting C++ file %s", str(file))
            subprocess.check_call(self.app.clang_format + ["-i", str(file)])

    @property
    def language(self):
        suffix = self.output.suffix
        if self.output.name == "CMakeLists.txt":
            return "cmake"
        if suffix in [".hpp", ".cpp"]:
            return "c++"
        elif suffix == ".cmake":
            return "cmake"
        raise Exception("Unexpected output file extension: " + suffix)

    def render(self):
        """Call Jinja renderer to create the output file and mark it read-only

        The output file is updated only if missing or if the new content
        is different.

        Returns:
            True if the output has been created or modified, False otherwise
        """
        updated = False
        content = self.app.jinja_template(self.input).render(**self.context)
        if self.output.exists():
            # render template in temporary file and update target file
            # ONLY if different (to save a lot of build time)
            fd, tmp_path = tempfile.mkstemp(dir=self.app.temp_dir)
            os.write(fd, content.encode("utf-8"))
            os.close(fd)
            self.format_output(Path(tmp_path))
            if not filecmp.cmp(str(self.output), tmp_path, shallow=False):
                self.logger.debug("previous output differs, updating it")
                # ensure destination file has write permissions
                mode = self.output.stat().st_mode
                rm_write_mask = stat.S_IWUSR | stat.S_IWGRP | stat.S_IWOTH
                self.output.chmod(mode | rm_write_mask)
                shutil.copy2(tmp_path, self.output)
                updated = True
        else:
            with self.output.open("w") as fd:
                fd.write(content)
            self.format_output(self.output)
            updated = True
        # remove write permissions on the generated file
        mode = self.output.stat().st_mode
        rm_write_mask = ~(stat.S_IWUSR | stat.S_IWGRP | stat.S_IWOTH)
        self.output.chmod(mode & rm_write_mask)
        return updated


def parse_args(args=None):
    """Parse arguments in command line and post-process them

    Arguments:
        args: arguments given in CLI
    """
    parser = argparse.ArgumentParser()
    parser.add_argument("--clang-format", help="Path to clang-format executable")
    parser.add_argument(
        "--clang-format-opts", help="clang-format options", action="append"
    )
    parser.add_argument("--base-dir", help="output root directory")
    parser.add_argument(
        "-v", "--verbosity", action="count", default=0, help="increase output verbosity"
    )
    args = parser.parse_args(args=args)

    # construct clang-format command line to use, if provided
    if args.clang_format:
        args.clang_format = [args.clang_format]
        if args.clang_format_opts:
            args.clang_format += args.clang_format_opts

    # destination directory to render templates
    args.base_dir = (
        Path(args.base_dir).resolve()
        if args.base_dir
        else Path(__file__).resolve().parent.parent
    )
    return args


def configure_logger(verbosity):
    """Prepare root logger

    Arguments:
        verbosity: integer greater than 0 indicating the verbosity level
    """
    level = logging.WARNING
    if verbosity == 1:
        level = logging.INFO
    elif verbosity > 1:
        level = logging.DEBUG
    logging.basicConfig(level=level, format=LOGGING_FORMAT)


def main(args=None):
    args = parse_args(args)
    configure_logger(args.verbosity)

    codegen = CodeGenerator(clang_format=args.clang_format, base_dir=args.base_dir)
    num_tasks = 0
    tasks_performed = []
    for task in codegen.workload():
        num_tasks += 1
        if task.execute():
            tasks_performed.append(task)

    if tasks_performed:
        LOGGER.info("Updated out-of-date files %i/%i", len(tasks_performed), num_tasks)
        padding = max(
            len(str(task.input.relative_to(codegen.this_dir)))
            for task in tasks_performed
        )
        for task in tasks_performed:
            input = task.input.relative_to(codegen.this_dir)
            LOGGER.debug(f"  %-{padding}s -> %s", input, task.output)
    else:
        LOGGER.info("Nothing to do")


if __name__ == "__main__":
    main()
