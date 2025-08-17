"""
Convert rst to a compressed dictionary suitable for NEURON + Python help system.
Run via:
>>> python3 parse_rst.py ./progref/ help_data.dat
"""

import sys
from pathlib import Path
import re


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
        if start >= 0:
            name = line[start + len(identifier) :].strip()


            indent_line = lines[i + 1]
            while not indent_line.strip():
                i += 1
                indent_line = lines[i + 1]
            start = 0
            while start < len(indent_line):
                if indent_line[start] != " ":
                    break
                start += 1

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
                    if body and body[-1] != "\n":
                        body.append("\n")
            
            # Check if body contains ".. tab:: Python" and extract only Python content
            python_tab_found = any(".. tab:: Python" in line for line in body)
            
            if python_tab_found:
                python_body = []
                in_python_tab = False
                
                for line in body:
                    if ".. tab:: Python" in line:
                        in_python_tab = True
                        continue
                    elif line.strip().startswith(".. tab::") and in_python_tab:
                        # Found another tab, stop collecting Python content
                        break
                    elif in_python_tab:
                        python_body.append(line)
                
                # Remove leading 4-space indentation repeatedly
                while True:
                    # Check if all non-empty lines begin with 4 spaces
                    can_remove_spaces = True
                    has_non_empty_lines = False
                    for line in python_body:
                        if line.strip():  # Only check non-empty lines
                            has_non_empty_lines = True
                            if not line.startswith("    "):
                                can_remove_spaces = False
                                break
                    
                    if can_remove_spaces and python_body and has_non_empty_lines:
                        # Remove 4 spaces from the beginning of each line
                        python_body = [line[4:] if line.startswith("    ") else line for line in python_body]
                    else:
                        break
                
                # Clean up consecutive newlines in the final result
                result = "\n".join(python_body).strip("\n")
                result = re.sub(r'\n[ \t]+\n', '\n\n', result)
                result = re.sub(r'\n{3,}', '\n\n', result)
                cls.help_dictionary[name] = result
            else:
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
