"""Run a command (ctest) and record the stacks of test processes that stall.

    python ci/stall_dump.py <seconds> -- ctest --output-on-failure

A hanging test is only ended by ctest's timeout, with no hint where it hung.
While the command runs, every 30 s this lists the current user's processes.
The first time a python, nrniv or special process has run longer than
<seconds>, it records the process's command line, py-spy's dump of its Python
stacks (with native frames on Linux) and gdb's or lldb's backtrace of every
thread. Each tool is optional; one that is missing or fails is noted. When the
command ends, the records are printed and this exits with the command's status.
"""

import os
import re
import shutil
import subprocess
import sys
import time

INTERPRETER = re.compile(r"^(python[0-9.]*|nrniv|special)$", re.IGNORECASE)


def seconds(etime):
    """ps etime, [[dd-]hh:]mm:ss, in seconds."""
    days, _, rest = etime.rpartition("-")
    parts = [int(p) for p in rest.split(":")]
    while len(parts) < 3:
        parts.insert(0, 0)
    hours, minutes, secs = parts
    return ((int(days or 0) * 24 + hours) * 60 + minutes) * 60 + secs


def long_running(threshold):
    out = subprocess.run(
        ["ps", "-U", str(os.getuid()), "-o", "pid=,etime=,command="],
        capture_output=True,
        text=True,
    ).stdout
    for line in out.splitlines():
        fields = line.split(None, 2)
        if len(fields) < 3:
            continue
        pid, etime, command = int(fields[0]), fields[1], fields[2]
        program = os.path.basename(command.split()[0])
        if pid == os.getpid() or not INTERPRETER.match(program):
            continue
        if seconds(etime) > threshold:
            yield pid, command


def run(command):
    """Output of command, through sudo first (attaching needs ptrace rights)."""
    lines = []
    for prefix in (["sudo", "-n"], []):
        try:
            result = subprocess.run(
                prefix + command, capture_output=True, text=True, timeout=120
            )
        except (OSError, subprocess.TimeoutExpired) as exc:
            lines.append(f"{' '.join(prefix + command)}: {exc}")
            continue
        lines.append(result.stdout + result.stderr)
        if result.returncode == 0:
            break
    return "\n".join(lines)


def stacks(pid, command):
    sections = [f"pid {pid}: {command}"]
    py_spy = shutil.which("py-spy") or shutil.which(
        "py-spy", path=os.path.dirname(sys.executable)
    )
    if py_spy:
        native = ["--native"] if sys.platform.startswith("linux") else []
        sections.append(run([py_spy, "dump", "--pid", str(pid), *native]))
    else:
        sections.append("py-spy not found")
    if shutil.which("gdb"):
        sections.append(
            run(
                ["gdb", "-batch", "-iex", "set debuginfod enabled off"]
                + ["-p", str(pid), "-ex", "thread apply all bt"]
            )
        )
    elif shutil.which("lldb"):
        sections.append(run(["lldb", "-p", str(pid), "--batch", "-o", "bt all"]))
    else:
        sections.append("no gdb or lldb")
    return "\n\n".join(sections)


def main():
    if len(sys.argv) < 4 or sys.argv[2] != "--":
        sys.exit(__doc__)
    threshold, command = int(sys.argv[1]), sys.argv[3:]
    child = subprocess.Popen(command)
    records = {}
    while child.poll() is None:
        for pid, line in long_running(threshold):
            if pid not in records:
                print(f"stall_dump: recording the stacks of pid {pid}", flush=True)
                records[pid] = stacks(pid, line)
        try:
            child.wait(timeout=30)
        except subprocess.TimeoutExpired:
            pass
    for pid, text in records.items():
        print(f"::group::Stacks of pid {pid}, still running after {threshold} s")
        print(text)
        print("::endgroup::", flush=True)
    sys.exit(child.returncode)


if __name__ == "__main__":
    main()
