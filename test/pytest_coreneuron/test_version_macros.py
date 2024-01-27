from neuron.tests.utils.strtobool import strtobool
import os
from neuron import coreneuron, h


def test_version_macros():
    """
    Use the special version_macros.mod mechanism to test the
    NRN_VERSION_X(maj, min, pat) preprocessor macros in VERBATIM blocks.
    """
    # Get the CMake PACKAGE_VERSION variable, which should always be in
    # major.minor.patch format. This should be baked into C++ as part of the main
    # NEURON build, not as part of nrnivmodl.
    ver = tuple(int(x) for x in h.nrnversion(0).split(".", 2))

    pc = h.ParallelContext()
    s = h.Section()
    s.insert("VersionMacros")
    coreneuron.enable = bool(
        strtobool(os.environ.get("NRN_CORENEURON_ENABLE", "false"))
    )
    coreneuron.verbose = True
    h.finitialize()
    pc.set_maxstep(10)
    pc.psolve(0.1)

    def t(name):
        vals = [getattr(seg.VersionMacros, name + "_result") for seg in s]
        assert len(vals) == 1
        return bool(vals[0])

    assert t("eq8_2_0") == (ver == (8, 2, 0))
    assert t("ne9_0_1") == (ver != (9, 0, 1))
    assert t("gt9_0_0") == (ver > (9, 0, 0))
    assert t("lt42_1_2") == (ver < (42, 1, 2))
    assert t("gteq10_4_7") == (ver >= (10, 4, 7))
    assert t("lteq8_1_0") == (ver <= (8, 1, 0))
    assert t("explicit_gteq8_2_0") == (ver >= (8, 2, 0))


if __name__ == "__main__":
    test_version_macros()
