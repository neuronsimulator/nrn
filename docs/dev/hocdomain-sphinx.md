# HOC Sphinx Domain

## Overview

The HOC Sphinx Domain is a Sphinx extension that allows to document HOC constructs in Sphinx.
Given the extra effort to create a full-fledged domain, we've decided to hack it from the Python domain.
Ideally we'd have a proper HOC domain, but that is an extra workload and we lack the required knowledge to build one.
See https://github.com/neuronsimulator/nrn/issues/1540 

## Hacking the Python Domain

It is sometimes required to re-hack the domain to make it work with the latest version of Sphinx. 
To that end, the following script can be used to generate the domain from the Python domain:

```bash
cd docs
python3 generate_hocdomain.py
```

This script generates a HOC domain from the one available in the sphinx package and writes it to:
    
    docs/domains/hocdomain.py

A comment is added at the top of the file to indicate that it is a generated file and the Sphinx version used to generate it.


## Sphinx Setup

Like all Sphinx setup, the HOC Sphinx Domain is registered in ``docs/conf.py``

```python
# 1st step: make docs/domains available to Sphinx
sys.path.insert(0, os.path.abspath("./domains"))

# ....

# 2nd step: import hocdomain
import hocdomain  # Sphinx HOC domain (hacked from the Python domain via docs/generate_hocdomain.py)

# ....

# 3rd step: setup the HOC domain in Sphinx
def setup(app):
    # ...
    # Set-up HOC domain
    hocdomain.setup(app)
```