"""Dentate CPU vs native-GPU prcellstate dump (progressive parity ladder).

Run from the reduced_dentate_native workdir (ACC special + datasets on PATH):

  # 1-rank end-of-run dump at tstop (default gid=500006 = first GC spike):
  NRN_TEST_TSTOP=5.5 NRN_PRCELLSTATE_GID=500006 NRN_GPU=0 \\
    special -notatty -python .../run_dentate_prcellstate.py
  NRN_TEST_TSTOP=5.5 NRN_PRCELLSTATE_GID=500006 NRN_GPU=1 \\
    special -notatty -python .../run_dentate_prcellstate.py

  # Phase checkpoints at last step ending at tstop:
  NRN_PRCELLSTATE_CHECKPOINT_T=5.5 NRN_GPU=1 ...

Env:
  NRN_TEST_TSTOP (default 10)
  NRN_TEST_MAX_CELLS (default 100)
  NRN_PRCELLSTATE_GID (default 500006)
  NRN_PRCELLSTATE_CHECKPOINT_T (>=0 arms fixed-step phases; default -1 = off)
  NRN_GPU 0=host fixed-step, 1=native GPU (default 0)
  NRN_PRCELLSTATE_TAG override suffix tag (default cpu|gpu)
  NRN_GPU_PERMUTE (default 2)

Writes <gid>_<tag>-t<tstop>.nrndat (and phase files if checkpoint armed).
Uses device→host download in prcellstate / checkpoint (native path).
"""
from __future__ import annotations

import os
import sys
from pathlib import Path

from neuron import h


def _patch_main_hoc(workdir: Path) -> Path:
    """Insert prcellstate arm + end dump around prun() without editing submodule."""
    src = workdir / "main.hoc"
    text = src.read_text()
    if "prcellstate_checkpoint" in text and "prcellstate wrote" in text:
        return src
    # Only arm phase checkpoints in HOC (before prun). End-of-run dump is done
    # from Python so the suffix tag is reliable (HOC strdef assignment is fragile).
    hook = r"""
// --- prcellstate diagnostic hook (injected by run_dentate_prcellstate.py) ---
if (!(name_declared("prcellstate_gid"))) { prcellstate_gid = -1 }
if (!(name_declared("prcellstate_checkpoint_t"))) { prcellstate_checkpoint_t = -1 }
if (prcellstate_gid >= 0 && prcellstate_checkpoint_t >= 0) {
  pnm.pc.prcellstate_checkpoint(prcellstate_gid, 0, prcellstate_checkpoint_t)
  if (pnm.myid == 0) {
    printf("prcellstate checkpoint armed: gid=%d t=%g\n", prcellstate_gid, prcellstate_checkpoint_t)
  }
}
prun()
// --- end prcellstate hook (end dump from Python) ---
"""
    if "\nprun()\n" not in text:
        raise RuntimeError("main.hoc: expected lone prun() to inject prcellstate hook")
    # Only replace the final simulation prun() (last occurrence).
    head, _, tail = text.rpartition("\nprun()\n")
    out = head + "\n" + hook + tail
    dst = workdir / "main_prcs.hoc"
    dst.write_text(out)
    return dst


def main() -> None:
    cwd = Path.cwd()
    if str(cwd) not in sys.path:
        sys.path.insert(0, str(cwd))

    h.nrnmpi_init()
    mytstop = float(os.environ.get("NRN_TEST_TSTOP", "10"))
    max_cells = int(os.environ.get("NRN_TEST_MAX_CELLS", "100"))
    gid = int(os.environ.get("NRN_PRCELLSTATE_GID", "500006"))
    ckpt_t = float(os.environ.get("NRN_PRCELLSTATE_CHECKPOINT_T", "-1"))
    use_gpu = os.environ.get("NRN_GPU", "0") not in ("0", "", "false", "False")
    tag = os.environ.get("NRN_PRCELLSTATE_TAG") or ("gpu" if use_gpu else "cpu")

    h(f"mytstop = {mytstop}")
    h(f"max_cells_per_type = {max_cells}")
    h("coreneuron = 0")
    h("gpu = 0")
    h(f"prcellstate_gid = {gid}")
    h(f"prcellstate_checkpoint_t = {ckpt_t}")

    if use_gpu:
        from neuron import gpu

        gpu.backend = "native"
        gpu.permute = int(os.environ.get("NRN_GPU_PERMUTE", "2"))
        gpu.enable = True

    main_hoc = _patch_main_hoc(cwd)
    # Minimal run.hoc equivalent loading our patched main
    h('load_file("defvar.hoc")')
    h('strdef parameters\nparameters="./parameters/Control.hoc"')
    h('default_var("coredat", "coredat")')
    h('default_var("outdir", ".")')
    h(f"default_var(\"mytstop\", {mytstop})")
    h("default_var(\"coreneuron\", 0)")
    h("default_var(\"gpu\", 0)")
    h(f"default_var(\"max_cells_per_type\", {max_cells})")
    h("use_coreneuron = coreneuron")
    h("use_gpu = gpu")
    h("dump_coreneuron_model = 0")
    h("max_cells_per_type_to_load = max_cells_per_type")
    h(
        """
        outdir = getcwd()
        sprint(coredat, "%s/%s", outdir, coredat)
        nrnpython("from commonutils import mkdir_p")
        strdef cmd
        sprint(cmd, "mkdir_p('%s')", coredat)
        nrnpython(cmd)
        sprint(cmd, "mkdir_p('%s/results')", outdir)
        nrnpython(cmd)
        """
    )
    # Re-apply mytstop after default_var (load order matches run.hoc)
    h(f"mytstop = {mytstop}")
    h(f"max_cells_per_type = {max_cells}")
    h(f"max_cells_per_type_to_load = {max_cells}")
    h(f"prcellstate_gid = {gid}")
    h(f"prcellstate_checkpoint_t = {ckpt_t}")
    if main_hoc.name == "main_prcs.hoc":
        main_hoc.unlink(missing_ok=True)
        main_hoc = _patch_main_hoc(cwd)
    h(f'load_file("{main_hoc.name}")')

    # End-of-run dump after psolve (native path downloads inside prcellstate).
    pc = h.ParallelContext()
    if gid >= 0 and int(pc.gid_exists(gid)):
        # Avoid trailing .0 mismatch: use same %g style as HOC when possible.
        t_now = float(h.t)
        suffix = f"{tag}-t{t_now:g}"
        pc.prcellstate(gid, suffix)
        if int(pc.id()) == 0:
            print(f"prcellstate wrote {gid}_{suffix}.nrndat (t={t_now})")
    elif int(pc.id()) == 0:
        print(f"prcellstate: gid {gid} not on this rank (nhost={int(pc.nhost())})")


if __name__ == "__main__":
    main()
