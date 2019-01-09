import jinja2
from pathlib import Path

from parser import LanguageParser
from ast_printer import *
import node_info


# parse nmodl definition file and get list of abstract nodes
nodes = LanguageParser("nmodl.yaml").parse_file()

# print various header and code files

AstForwardDeclarationPrinter(
    "../ast/ast_decl.hpp",
    "",
    nodes).write()

AstDeclarationPrinter(
    "../ast/ast.hpp",
    "",
    nodes).write()

AstDefinitionPrinter(
    "../ast/ast.cpp",
    "",
    nodes).write()

templates = Path(__file__).parent / 'templates'
basedir = Path(__file__).resolve().parent.parent

env = jinja2.Environment(loader=jinja2.FileSystemLoader(str(templates)),
                         trim_blocks=True,
                         lstrip_blocks=True)
env.filters['snake_case'] = to_snake_case

for fn in templates.glob('*.[ch]pp'):
    filename = basedir / ('visitors' if 'visitor' in fn.name else 'ast') / fn.name
    template = env.get_template(fn.name)
    with filename.open('w') as fd:
        fd.write(template.render(nodes=nodes, node_info=node_info))
