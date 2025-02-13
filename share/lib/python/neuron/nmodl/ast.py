"""
Module for vizualization of NMODL abstract syntax trees (ASTs).
"""

import getpass
import json
import os
import sys
import tempfile
import webbrowser
from shutil import copytree


if sys.version_info >= (3, 9):
    from importlib.resources import files
else:
    from importlib_resources import files

from ._nmodl import to_json
from ._nmodl.ast import *  # noqa


def view(nmodl_ast):
    """Visualize given NMODL AST in web browser

    In memory representation of AST can be converted to JSON
    form and it can be visualized using AST viewer implemented
    using d3.js.

    Args:
        nmodl_ast: AST object of nmodl file or string

    Returns:
        None
    """

    path = files("nmodl") / "ext/viz"
    if not path.is_dir():
        raise FileNotFoundError("Could not find sample mod files")

    work_dir = os.path.join(tempfile.gettempdir(), getpass.getuser(), "nmodl")

    # first copy necessary files to temp work directory
    copytree(path, work_dir, dirs_exist_ok=True)

    # prepare json data
    json_data = json.loads(to_json(nmodl_ast, True, True, True))
    with open(os.path.join(work_dir, "ast.js"), "w", encoding="utf-8") as outfile:
        outfile.write(f"var astRoot = {json.dumps(json_data)};")

    # open browser with ast
    url = "file://" + os.path.join(work_dir, "index.html")
    webbrowser.open(url, new=1, autoraise=True)
