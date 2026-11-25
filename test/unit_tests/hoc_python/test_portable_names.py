"""The interface asks libnrniv only for functions that neuronapi.h declares: they are documented,
and every compiler spells them the same. This reads the sources."""

import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[3]
HEADER = (ROOT / "src/nrniv/neuronapi.h").read_text()
DECLARED = set(re.findall(r"\b(nrn_\w+)\s*\(", HEADER))
LOOKUP = r"""_bind\(\s*"(\w+)"|attr\(libnrniv,\s*"(\w+)"|(?<!")libnrniv\.(\w+)|^    "(nrn_\w+)",$"""
# A mangled name, a variable, a structure's layout, another library, a name that is not literal
PRIVATE = r"""["']_Z\w+|["']\?\w+@|\bin_dll\b|\bfrom_address\b|\b(?:C|Win|Ole)DLL\(|libnrniv\[|"""
PRIVATE += r"""(?<!def )_bind\((?!\s*")|attr\((?:\w+\.)?libnrniv, (?!"|name\))"""


def problems(source):
    source = re.sub(r"#.*|\"\"\"[\s\S]*?\"\"\"", "", source)
    names = {"".join(groups) for groups in re.findall(LOOKUP, source, re.M)}
    return sorted(names - DECLARED - {"_name"}) + re.findall(PRIVATE, source)


def test_interface():
    files = sorted((ROOT / "share/lib/python/neuron/_interface").glob("*.py"))
    found = {f.name: problems(f.read_text(encoding="utf-8")) for f in files}
    assert files and not {k: v for k, v in found.items() if v}


def test_slips_are_found():
    assert not problems('_bind("nrn_symbol", None)\nlibnrniv._name\nx = "?x"')
    for slip in (
        'c_int.in_dll(libnrniv, "stoprun")\n_bind("hoc_pushx", 0)\n_bind(n, 0)\nlibnrniv.x(1.0)\n'
        'libnrniv["x"]\nb"_ZN6neuron"\n"?x@@3PEA"\nc_void_p.from_address(h + 8)\nctypes.CDLL(None)'
    ).split("\n"):
        assert problems(slip), slip
