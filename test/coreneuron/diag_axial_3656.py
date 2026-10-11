"""Run the first test_axial sample and print every im versus i_membrane_.

Set up by .github/workflows/axial-3656-diag.yml. The process cwd must be the
nrnivmodl directory so libnrnmech loads once. AXIAL_3656_DIAG is set here.
"""

import os
import platform
import sys
from pathlib import Path

os.environ["AXIAL_3656_DIAG"] = "1"
sys.path.insert(0, str(Path(__file__).resolve().parent))

print("AXIAL3656 python", sys.version, flush=True)
print("AXIAL3656 platform", platform.platform(), platform.mac_ver(), flush=True)

import test_pointer

test_pointer.test_axial()
