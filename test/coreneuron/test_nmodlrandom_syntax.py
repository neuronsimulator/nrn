from neuron import h
import subprocess, pathlib


# default args generate accepted nmodl string
def modfile(
    s0=":s0",
    s1="RANDOM rv1, rv2",
    s2=":s2",
    s3=":s3",
    s4="4",
    s5="random_negexp(rv1, 1.0)",
):
    txt = """
: temp.mod file with format elements to test for RANDOM syntax errors

%s : 0 error if randomvar is mentioned

NEURON {
  SUFFIX temp
  RANGE x1
  %s : 1 declare randomvars
}

ASSIGNED {
  x1
  %s : 2 error if randomvar
}

BEFORE STEP {
  %s = 5 : 3 cannot assign to randomvar
  x1 = %s : 4 cannot evaluate randomvar
  %s : 5 random_function accepted but ranvar must be first arg
}

FUNCTION foo(arg) {
  foo = arg
}
""" % (
        s0,
        s1,
        s2,
        s3,
        s4,
        s5,
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
        pathlib.Path.unlink("temp.mod")
        pathlib.Path.unlink("temp.cpp")
    else:
        print("chk_nmodl ", program, " return code ", result.returncode)
        print(txt)
        print(result.stderr)
        print(result.stdout)
    return (result.returncode == 0) == rcode


def test_syntax():
    for program in ["nocmodl"]:
        assert chk_nmodl(modfile(), program, rcode=True)
        assert chk_nmodl(modfile(s0="ASSIGNED{rv1}"), program)
        assert chk_nmodl(modfile(s2="rv1"), program)
        assert chk_nmodl(modfile(s3="rv1"), program)
        assert chk_nmodl(modfile(s4="rv1"), program)
        assert chk_nmodl(modfile(s5="foo(rv1)"), program)
        assert chk_nmodl(modfile(s5="random_negexp()"), program)
        assert chk_nmodl(modfile(s5="random_negexp(1.0)"), program)


if __name__ == "__main__":
    test_syntax()
