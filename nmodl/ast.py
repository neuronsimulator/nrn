from ._nmodl.ast import *  # noqa
from .config import *

def ast_view(nmodl_ast):
    """Visualize given NMODL AST by converting to JSON form"""
    from ._nmodl import to_json
    from distutils.dir_util import copy_tree
    import getpass
    import json
    import os
    import tempfile
    import webbrowser

    installed_viz_tool = os.path.join(PROJECT_INSTALL_DIR, "share", "viz")
    if os.path.exists(installed_viz_tool):
        viz_tool_dir = installed_viz_tool
    else:
        viz_tool_dir = os.path.join(PROJECT_SOURCE_DIR, "share", "viz")

    work_dir = os.path.join(tempfile.gettempdir(), getpass.getuser(), "nmodl")

    # first copy necessary files to temp work directory
    copy_tree(viz_tool_dir, work_dir)

    # prepare json data
    with open(os.path.join(work_dir, 'ast.js'), 'w') as outfile:
        json_data = json.loads(to_json(nmodl_ast, True, True, True))
        outfile.write('var astRoot = %s;' % json.dumps(json_data))

    # open browser with ast
    url = 'file://' + os.path.join(work_dir, "index.html")
    webbrowser.open(url, new=1, autoraise=True)

