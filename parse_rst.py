"""
Convert rst to a compressed dictionary suitable for NEURON + Python
help system.

Run via:

python parse_rst.py `find . -name \*.rst -print`
"""

import sys

help_dictionary = {}

filenames = sys.argv[1:]

def handle_identifier(lines, i, identifier):
    identifier = '.. %s::' % identifier
    line = lines[i]
    start = line.find(identifier)
    #print line, identifier, start
    #print identifier
    if start >= 0:
        name = line[start + len(identifier):].strip()
        
        print '%s -- %s' % (name, identifier)
        
        indent_line = lines[i + 1]
        while not indent_line.strip():
            i += 1
            indent_line = lines[i + 1]
        start = 0
        while start < len(indent_line):
            if indent_line[start] != ' ': break
            start += 1
        
        # TODO: store the body text
        body = []
        while i < len(lines) - 1:
            i += 1
            if lines[i].strip():
                if lines[i][:start] == indent_line[:start]:
                    next_line = lines[i][start:]
                    if next_line[-1] == '\n':
                        next_line = next_line[: -1]
                    body.append(next_line)
                else:
                    break
            else:
                if not body or body[-1] != '\n':
                    body.append('\n')
        help_dictionary[name] = '\n'.join(body)
            
        
for filename in filenames:
    with open(filename) as f:
        lines = []
        for line in f:
            if line[-1] == '\n':
                line = line[:-1]
            lines.append(line)

    i = 0
    while i < len(lines):
        for kind in ['method', 'data', 'class', 'function']:
            handle_identifier(lines, i, kind)
        i += 1

import cPickle
import zlib
compressed = zlib.compress(cPickle.dumps(help_dictionary))
with open('help_data.dat', 'wb') as f:
    f.write(compressed)

