"""Startup options the legacy extension's PyInit_hoc passed to ivocmain.

Used when this package is NEURON's own interface (``neuron._interface``), so
``from neuron import h`` starts NEURON the way the compiled module did
(src/nrnpython/inithoc.cpp:261-380): ``__main__.neuron_options``,
``NEURON_MODULE_OPTIONS``, the mechanism library NEURON's tests rely on being
loaded from ``<arch>/`` in the working directory, and ``NEURON_NFRAME``.
Standalone myneuron keeps its own minimal argv.
"""

import os
import platform
import sys


def _add(args, name, value=None):
    # Native add_arg skips a name already present anywhere in argv.
    if name in args:
        return
    args.append(name)
    if value is not None:
        args.append(value)


def _main_options(args, write):
    """Add ``__main__.neuron_options``; return True for -print-options."""
    options = getattr(sys.modules.get("__main__"), "neuron_options", None)
    if options is None:
        return False
    if not isinstance(options, dict):
        write("__main__.neuron_options is not a dict\n")
        return False
    print_options = False
    for key, value in options.items():
        if not isinstance(key, (str, bytes)) or not (
            value is None or isinstance(value, (str, bytes))
        ):
            write("A neuron_options key:value is not a string:string or string:None\n")
            continue
        key = key.decode() if isinstance(key, bytes) else key
        value = value.decode() if isinstance(value, bytes) else value
        if key == "-print-options":
            print_options = True
            continue
        _add(args, key, value)
    return print_options


def split_options(text):
    """Port of add_space_separated_options: escapes, '...' and "..."."""
    tokens, i, n = [], 0, len(text)
    while i < n:
        while i < n and text[i].isspace():
            i += 1
        if i >= n:
            break
        token = []
        while i < n and not text[i].isspace():
            char = text[i]
            i += 1
            if char == "\\" and i < n and (text[i].isspace() or text[i] in "\"'"):
                token.append(text[i])
                i += 1
            elif char in "\"'":
                while i < n and text[i] != char:
                    if text[i] == "\\" and i + 1 < n and text[i + 1] == char:
                        i += 1
                    token.append(text[i])
                    i += 1
                i += 1  # closing quote (or end of string)
            else:
                token.append(char)
        tokens.append("".join(token))
    return tokens


def mechanism_library(cwd="."):
    """Relative path of the auto-loaded mechanism library, or None."""
    if sys.platform == "win32":
        prefix, suffix = "", ".dll"
    elif sys.platform == "darwin":
        prefix, suffix = "lib", ".dylib"
    else:
        prefix, suffix = "lib", ".so"
    path = f"{platform.machine()}/{prefix}nrnmech{suffix}"
    return path if os.path.isfile(os.path.join(cwd, path)) else None


def startup_options(write=sys.stdout.write, environ=os.environ):
    """Arguments after argv[0], in the order PyInit_hoc adds them."""
    args = []
    print_options = _main_options(args, write)
    for token in split_options(environ.get("NEURON_MODULE_OPTIONS", "")):
        if token == "-print-options":
            print_options = True
        else:
            _add(args, token)
    if environ.get("NEURON_INIT_MPI") == "1" or "-mpi" in args:
        raise ImportError(
            "MPI initialization through `import neuron` is not supported by "
            "this interface yet; unset NEURON_INIT_MPI and drop -mpi"
        )
    library = mechanism_library()
    if library:
        _add(args, "-dll", library)
    nframe = environ.get("NEURON_NFRAME")
    if nframe is not None:
        if nframe.isdigit() and int(nframe) > 0:
            _add(args, "-NFRAME", nframe)
        elif nframe.lstrip("-").isdigit():
            write("NEURON_NFRAME env value must be positive\n")
        else:
            write("NEURON_NFRAME env value is invalid!\n")
    if print_options:
        write("ivocmain options:" + "".join(f" '{arg}'" for arg in args) + "\n")
    return args
