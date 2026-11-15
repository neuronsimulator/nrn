"""Run a command (ctest) and record the stacks of test processes that stall.

    python ci/stall_dump.py <seconds> -- ctest --output-on-failure

A hanging test is only ended by ctest's timeout, with no hint where it hung.
While the command runs, every 30 s this looks at the command's descendant
processes. The first time a python, nrniv or special process among them has
run longer than <seconds>, it prints the process's command line, py-spy's dump
of its Python stacks (with native frames on Linux) and gdb's or lldb's
backtrace of every thread, in a collapsed group. Each tool is optional; one
that is missing, fails or takes too long is noted. This exits with the
command's status.
"""

import os
import re
import shutil
import subprocess
import sys
import time

INTERPRETER = re.compile(r"^(python[0-9.]*|nrniv|special)$", re.IGNORECASE)
TOOL_SECONDS = 90


def seconds(etime):
    """ps etime, [[dd-]hh:]mm:ss, in seconds."""
    days, _, rest = etime.rpartition("-")
    parts = [int(p) for p in rest.split(":")]
    while len(parts) < 3:
        parts.insert(0, 0)
    hours, minutes, secs = parts
    return ((int(days or 0) * 24 + hours) * 60 + minutes) * 60 + secs


def long_running(root, threshold):
    """(pid, command) of root's descendants that look stalled."""
    out = subprocess.run(
        ["ps", "-U", str(os.getuid()), "-o", "pid=,ppid=,etime=,command="],
        capture_output=True,
        text=True,
    ).stdout
    procs = {}
    for line in out.splitlines():
        fields = line.split(None, 3)
        if len(fields) == 4:
            procs[int(fields[0])] = (int(fields[1]), fields[2], fields[3])
    for pid, (ppid, etime, command) in procs.items():
        ancestor, seen = ppid, set()
        while ancestor in procs and ancestor != root and ancestor not in seen:
            seen.add(ancestor)
            ancestor = procs[ancestor][0]
        if ancestor != root:
            continue
        program = os.path.basename(command.split()[0])
        if INTERPRETER.match(program) and seconds(etime) > threshold:
            yield pid, command


def run(command):
    """Output of command, through sudo first (attaching needs ptrace rights).

    A command run through sudo can't be killed from here, so each attempt is
    limited with timeout(1) where there is one, and one that still overruns is
    left behind rather than waited for.
    """
    limit = ["timeout", str(TOOL_SECONDS)] if shutil.which("timeout") else []
    lines = []
    for prefix in (["sudo", "-n"], []):
        argv = prefix + limit + command
        try:
            child = subprocess.Popen(
                argv, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True
            )
        except OSError as exc:
            lines.append(f"{' '.join(argv)}: {exc}")
            continue
        try:
            output, _ = child.communicate(timeout=TOOL_SECONDS + 10)
        except subprocess.TimeoutExpired:
            try:
                child.kill()
            except OSError:
                pass
            lines.append(f"{' '.join(argv)}: no answer after {TOOL_SECONDS + 10} s")
            continue
        lines.append(output)
        if child.returncode == 0:
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
    recorded = set()
    while child.poll() is None:
        try:
            for pid, line in long_running(child.pid, threshold):
                if pid in recorded:
                    continue
                recorded.add(pid)
                text = stacks(pid, line)
                print(
                    f"::group::Stacks of pid {pid}, still running after {threshold} s"
                )
                print(text)
                print("::endgroup::", flush=True)
        except Exception as exc:  # never leave the command unattended
            print(f"stall_dump: {exc!r}", flush=True)
        try:
            child.wait(timeout=30)
        except subprocess.TimeoutExpired:
            pass
    sys.exit(child.returncode)


if __name__ == "__main__":
    main()
