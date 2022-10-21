import os
import pkgutil
import re
import sphinx

"""
This script generates a HOC domain from the one available in the sphinx package and writes it to:
    
    docs/domains/hocdomain.py

Ideally we'd have a proper HOC domain, but that is an extra workload and we lack the required knowledge to build one.
See https://github.com/neuronsimulator/nrn/issues/1540 


"""


def generate_hoc_domain_from_py():
    # Include Sphinx version for robustness
    gen_from = "# generated from 'sphinx/domains/python.py' @ Sphinx {}\n".format(
        sphinx.__version__
    )
    print("Hacking docs/domains/hocdomain.py " + gen_from)

    # Retrieve Python domain implementation
    py_domain = gen_from + pkgutil.get_data("sphinx.domains", "python.py").decode(
        "utf-8"
    )

    # regex replacements from Python to HOC domain (working Sphinx version 4.4.0)
    hoc_dict = {
        # Strings used for indexing.
        # For future updates, tthe generated index must be visually checked and x-reffed against previous index
        " (%s ": " (HOC %s ",
        "'%s (": "'%s (HOC ",
        "'built-in function'": "'HOC built-in function'",
        "(built-in variable)": "(HOC built-in variable)",
        # for Sphinx internals, basically renaming Python names of objects, types, clasess ..
        "'python": "'hoc",
        "python_": "hoc_",
        "Python": "HOC",
        "Py": "HOC",
        "'py": "'hoc",
        "py_": "hoc_",
    }

    # Create a regular expression  from the dictionary keys
    regex = re.compile("(%s)" % "|".join(map(re.escape, hoc_dict.keys())))

    # For each match, look-up corresponding value in dictionary
    hoc_domain = regex.sub(
        lambda mo: hoc_dict[mo.string[mo.start() : mo.end()]], py_domain
    )

    # Write out HOC domain
    with open(
        os.path.join(os.path.abspath("."), "domains/hocdomain.py"), "w"
    ) as hoc_py:
        hoc_py.writelines(hoc_domain)

    print("Done.")


if __name__ == "__main__":
    generate_hoc_domain_from_py()
