from neuron import h
import subprocess
from pathlib import Path
import os


# default args generate accepted nmodl string
def modfile(
    s0=":s0",
    s1="RANDOM rv1, rv2",
    s2=":s2",
    s3=":s3",
    s4="x1 = random_negexp(rv1, 1.0)",
    s5=":s5",
    s6="foo = random_negexp(rv1, 1.0)",
):
    txt = """
: temp.mod file with format elements to test for RANDOM syntax errors

%s : 0 error if randomvar is mentioned

NEURON {
  SUFFIX temp
  RANGE x1
  %s : 1 declare randomvars
}

%s : 2 error if randomvar is mentioned

ASSIGNED {
  x1
}

BEFORE STEP {
  %s : 3 error if assign or eval a randomvar
  %s : 4 random_function accepted but ranvar must be first arg
}

FUNCTION foo(arg) {
  %s : 5  LOCAL ranvar makes it a double in this scope
  %s : 6  random_function accepted but ranvar must be first arg
  foo = arg
}
""" % (
        s0,
        s1,
        s2,
        s3,
        s4,
        s5,
        s6,
    )
    return txt


def run(cmd):
    result = subprocess.run(cmd, capture_output=True)
    return result


def chk_nmodl(txt, program="nocmodl", rcode=False):
    f = open("temp.mod", "w")
    f.write(txt)
    f.close()
    result = run([program, "temp.mod"])
    ret = (result.returncode == 0) == rcode
    if ret:
        Path("temp.mod").unlink(missing_ok=True)
        Path("temp.cpp").unlink(missing_ok=True)
    else:
        print("chk_nmodl ", program, " return code ", result.returncode)
        print(txt)
        print(result.stderr.decode())
        print(result.stdout.decode())
    return (result.returncode == 0) == rcode


def test_syntax():
    # nmodl could be external installation (not in PATH)
    nmodl_binary = os.environ.get("NMODL_BINARY", "nmodl")
    for program in ["nocmodl", nmodl_binary]:
        foo = False
        assert chk_nmodl(modfile(), program, rcode=True)
        assert chk_nmodl(modfile(s0="ASSIGNED{rv1}"), program)
        foo = True if program == "nocmodl" else False
        assert chk_nmodl(modfile(s0="LOCAL rv1"), program, rcode=foo)
        assert chk_nmodl(modfile(s2="ASSIGNED{rv1}"), program)
        assert chk_nmodl(modfile(s2="LOCAL rv1"), program)
        assert chk_nmodl(modfile(s3="rv1 = 1"), program)
        assert chk_nmodl(modfile(s3="x1 = rv1"), program)
        assert chk_nmodl(modfile(s4="foo(rv1)"), program)
        assert chk_nmodl(modfile(s4="random_negexp()"), program)
        assert chk_nmodl(modfile(s4="random_negexp(1.0)"), program)
        assert chk_nmodl(modfile(s5="LOCAL rv1"), program)
        assert chk_nmodl(modfile(s4="random_negexp(rv1, rv2)"), program)


if __name__ == "__main__":
    test_syntax()
