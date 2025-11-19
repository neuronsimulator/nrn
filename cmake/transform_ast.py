#!/usr/bin/env python3

"""
Replace not-yet-built module imports with `None`.
"""

import argparse
import ast

from typing import Iterable


class RewriteImports(ast.NodeTransformer):
    """
    Rewrite imports that match a certain criteria and replace them with `None`.
    """

    def __init__(self, prefixes: Iterable[str]):
        self.prefixes: list[str] = list(prefixes)
        super().__init__()

    def visit_Import(self, node):  # pylint: disable=invalid-name
        """
        Replace any instances of `import <prefix>` or `import <prefix> as` with `None`.

        Notes
        -----
        If importing multiple modules on a single line, the method will only
        replace those which are mentioned in `prefixes`.
        """
        new_names = [
            alias
            for alias in node.names
            if not any(alias.name.startswith(prefix) for prefix in self.prefixes)
        ]

        if not new_names:
            # If all imports were filtered out, replace with None statement
            return ast.Expr(value=ast.Constant(None))

        # Update the node with the filtered imports
        node.names = new_names
        return node

    def visit_ImportFrom(self, node):  # pylint: disable=invalid-name
        """
        Replace any instances of `from <prefix> ...` with `None`.
        """
        if node.module and any(
            node.module.startswith(prefix) for prefix in self.prefixes
        ):
            return ast.Expr(value=ast.Constant(None))
        return node


def main():  # pylint: disable=missing-function-docstring
    parser = argparse.ArgumentParser(
        description="Replace not-yet-built module imports with `None` in a Python file.",
    )
    parser.add_argument(
        "file",
        help="The file to parse",
    )
    parser.add_argument(
        "modules",
        nargs="+",
        help="List of module prefixes to replace",
    )
    parser.add_argument(
        "-o",
        "--output",
        type=str,
        help="Where to output the result (default: stdout).",
    )
    args = parser.parse_args()
    with open(args.file, "r", encoding="utf-8") as f:
        source = f.read()

    tree = ast.parse(source)
    modified_tree = RewriteImports(args.modules).visit(tree)
    modified_source = ast.unparse(modified_tree)
    if not args.output:
        print(modified_source)
    else:
        with open(args.output, "w", encoding="utf-8") as f:
            f.write(modified_source)


if __name__ == "__main__":
    main()
