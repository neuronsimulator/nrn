"""
Convert rst to a compressed dictionary suitable for NEURON + Python help system.
Run via:
>>> python3 parse_rst.py ./python/ help_data.dat
"""

import os
import sys
from pathlib import Path


class ParseRst(object):
    help_dictionary = {}

    def __init__(self, rst_path, out_file):
        self._rst_path = rst_path
        self._out_file = out_file
        self._filenames = Path(self._rst_path).glob("**/*.rst")

    @classmethod
    def handle_identifier(cls, lines, i, identifier):
        identifier = ".. %s::" % identifier
        line = lines[i]
        start = line.find(identifier)
        # print line, identifier, start
        # print identifier
        if start >= 0:
            name = line[start + len(identifier) :].strip()

            # print('%s -- %s' % (name, identifier))

            indent_line = lines[i + 1]
            while not indent_line.strip():
                i += 1
                indent_line = lines[i + 1]
            start = 0
            while start < len(indent_line):
                if indent_line[start] != " ":
                    break
                start += 1

            # TODO: store the body text
            body = []
            while i < len(lines) - 1:
                i += 1
                if lines[i].strip():
                    if lines[i][:start] == indent_line[:start]:
                        next_line = lines[i][start:]
                        if next_line[-1] == "\n":
                            next_line = next_line[:-1]
                        body.append(next_line)
                    else:
                        break
                else:
                    if not body or body[-1] != "\n":
                        body.append("\n")
            cls.help_dictionary[name] = "\n".join(body)

    def parse(self):
        for filename in self._filenames:
            with open(str(filename), encoding="utf-8") as f:
                lines = []
                for line in f:
                    if line[-1] == "\n":
                        line = line[:-1]
                    lines.append(line)
            i = 0
            while i < len(lines):
                for kind in ["method", "data", "class", "function"]:
                    ParseRst.handle_identifier(lines, i, kind)
                i += 1


if __name__ == "__main__":
    if len(sys.argv) == 3:
        rst_path = sys.argv[1]
        out_file = sys.argv[2]
    else:
        print("usage: python3 parse_rst.py <rst_folder_path> <output_file>")
        exit(1)

    try:
        ParseRst(rst_path, out_file).parse()
        with open(out_file, "wb") as f:
            import pickle
            import zlib

            compressed = zlib.compress(pickle.dumps(ParseRst.help_dictionary))
            f.write(compressed)
    except Exception:
        import traceback

        print(traceback.format_exc())
        exit(1)
