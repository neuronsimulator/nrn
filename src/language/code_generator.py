import jinja2
from pathlib import Path

from parser import LanguageParser
import node_info
import utils

# parse nmodl definition file and get list of abstract nodes
nodes = LanguageParser("nmodl.yaml").parse_file()

templates = Path(__file__).parent / 'templates'
basedir = Path(__file__).resolve().parent.parent

env = jinja2.Environment(loader=jinja2.FileSystemLoader(str(templates)),
                         trim_blocks=True,
                         lstrip_blocks=True)
env.filters['snake_case'] = utils.to_snake_case

for fn in templates.glob('*.[ch]pp'):
    filename = basedir / ('visitors' if 'visitor' in fn.name else 'ast') / fn.name
    template = env.get_template(fn.name)
    with filename.open('w') as fd:
        fd.write(template.render(nodes=nodes, node_info=node_info))
