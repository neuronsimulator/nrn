from neuron import h

def test_mod_legacy():
  ut = h.UnitsTest()
  h.nrnunit_use_legacy(1)
  h.finitialize()
  names =  ["mole", "e", "faraday", "planck", "hbar", "gasconst"]
  legacy_hex_values = ["0x1.fe18fef60659ap+78", "0x1.7a4e7164efbbcp-63",
    "0x1.78e54cccccccdp+16", "0x1.b85f8c5445f02p-111", "0x1.18779454e3d48p-113",
    "0x1.0a10624dd2f1bp+3"]
  for i, n in enumerate(names):
    val = eval("ut." + n)
    print ("%s = %s (%s)" %(n, str(val.hex()), str(val)))
    legacy_value = float.fromhex(legacy_hex_values[i])
    assert (val == legacy_value)
  h.nrnunit_use_legacy(0)
  h.finitialize()
  assert (ut.mole != float.fromhex(legacy_hex_values[0]))

if __name__ == "__main__":
  test_mod_legacy()
