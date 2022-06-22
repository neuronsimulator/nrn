from neuron import h


def switch_units(legacy):
    try:
        h.nrnunit_use_legacy(legacy)
    except:
        pass


def test_mod_legacy():
    s = h.Section()
    ut = h.UnitsTest(s(0.5))
    h.ion_style("na_ion", 1, 2, 1, 1, 0, sec=s)
    switch_units(1)
    h.finitialize()
    names = ["mole", "e", "faraday", "planck", "hbar", "gasconst"]
    legacy_hex_values = [
        "0x1.fe18fef60659ap+78",
        "0x1.7a4e7164efbbcp-63",
        "0x1.78e54cccccccdp+16",
        "0x1.b85f8c5445f02p-111",
        "0x1.18779454e3d48p-113",
        "0x1.0a10624dd2f1bp+3",
    ]
    for i, n in enumerate(names):
        val = eval("ut." + n)
        print("%s = %s (%s)" % (n, str(val.hex()), str(val)))
        legacy_value = float.fromhex(legacy_hex_values[i])
        assert val == legacy_value
    assert ut.avogadro == float.fromhex(legacy_hex_values[0])
    ghk_std = h.ghk(-50, 0.001, 10, 2)
    erev_std = h.nernst(s(0.5).nai, s(0.5).nao, 1)
    assert ut.ghk == ghk_std
    assert ut.erev == erev_std
    switch_units(0)
    h.finitialize()
    ghk_std = h.ghk(-50, 0.001, 10, 2)
    erev_std = h.nernst(s(0.5).nai, s(0.5).nao, 1)
    assert ut.mole != float.fromhex(legacy_hex_values[0])
    assert ut.e * ut.avogadro == ut.faraday
    assert abs(ut.faraday - h.FARADAY) < 1e-10
    assert ut.gasconst == h.R
    assert ut.k * ut.avogadro == ut.gasconst
    assert abs(ut.planck - ut.hbar * 2.0 * h.PI) < 1e-49
    assert ut.avogadro == h.Avogadro_constant
    assert ut.ghk == h.ghk(-50, 0.001, 10, 2)
    assert ut.erev == h.nernst(s(0.5).nai, s(0.5).nao, 1)


def test_hoc_legacy():
    switch_units(1)  # legacy
    print("R = %s" % str(h.R))
    print("FARADAY = %s" % str(h.FARADAY))
    celsius = 6.3
    ghk = h.ghk(30, 0.01, 10, 1)  # nernst requires a Section to get voltage
    print("ghk = %s" % str(ghk))

    assert h.R == 8.31441
    assert h.FARADAY == 96485.309
    assert ghk == -483.7914803097116

    switch_units(0)  # Modern
    print("R = %s" % str(h.R))
    print("FARADAY = %s" % str(h.FARADAY))
    ghk = h.ghk(30, 0.01, 10, 1)
    print("ghk = %s" % str(ghk))
    assert h.R == 8.31446261815324
    assert ghk == -483.8380971405879


def test_env_legacy():
    for i in [0, 1]:
        a = None
        try:  # test NRNUNIT_USE_LEGACY not whether we can successfully run a subprocess
            import sys, subprocess

            a = (
                "NRNUNIT_USE_LEGACY=%d %s -c 'from neuron import h; print (h.nrnunit_use_legacy())'"
                % (i, sys.executable)
            )
            a = subprocess.check_output(a, shell=True)
            a = int(float(a.decode().split()[0]))
        except:
            pass
        assert a == i


def test_errorcode():
    import sys, subprocess

    process = subprocess.run('nrniv -c "1/0"', shell=True)
    assert process.returncode > 0

    process = subprocess.run(
        '{} -c "from neuron import h; h.sqrt(-1)"'.format(sys.executable), shell=True
    )
    assert process.returncode > 0


if __name__ == "__main__":
    test_mod_legacy()
    test_hoc_legacy()
    test_env_legacy()
