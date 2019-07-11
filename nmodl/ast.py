from ._nmodl.ast import *  # noqa
from pkg_resources import *

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
    from ._nmodl import to_json
    from distutils.dir_util import copy_tree
    import getpass
    import json
    import os
    import tempfile
    import webbrowser

    resource = "ext/viz"
    if resource_exists(__name__, resource) and resource_isdir(__name__, resource):
        installed_viz_tool = resource_filename(__name__, resource)
    else:
        raise FileNotFoundError("Could not find sample mod files")

    work_dir = os.path.join(tempfile.gettempdir(), getpass.getuser(), "nmodl")

    # first copy necessary files to temp work directory
    copy_tree(installed_viz_tool, work_dir)

    # prepare json data
    with open(os.path.join(work_dir, 'ast.js'), 'w') as outfile:
        json_data = json.loads(to_json(nmodl_ast, True, True, True))
        outfile.write('var astRoot = %s;' % json.dumps(json_data))

    # open browser with ast
    url = 'file://' + os.path.join(work_dir, "index.html")
    webbrowser.open(url, new=1, autoraise=True)

