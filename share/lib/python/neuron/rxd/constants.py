# Avogadro's number (approximation before 2019 redefinition)
NA_legacy = 6.02214129e23
NA_modern = 6.02214076e23

def NA():
  try:
    from neuron import h
    val = NA_legacy if h.nrnunit_use_legacy() else NA_modern
  except:
    val = NA_modern
  return val

