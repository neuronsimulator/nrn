import os
import sys

# Python import of neuron starts IV unless this is set. nrniv.exe -dll GUI
# tests must pop it in their own subprocess.
os.environ.setdefault("NEURON_MODULE_OPTIONS", "-nogui")

# Python 3.8+ on Windows does not search PATH for dependent DLLs.
if sys.platform == "win32":
    for p in os.environ.get("PATH", "").split(os.pathsep):
        if p and os.path.isfile(os.path.join(p, "nrniv.dll")):
            os.add_dll_directory(p)
            break
