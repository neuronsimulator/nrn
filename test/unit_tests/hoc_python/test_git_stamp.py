"""Git version stamp stays in the build tree unless source stamping is requested."""

import os
import stat
import subprocess
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parents[3]
GIT_INFO = REPO_ROOT / "cmake" / "NrnGitInfo.cmake"
MACRO_HELPER = REPO_ROOT / "cmake" / "MacroHelper.cmake"

LISTS = """\
cmake_minimum_required(VERSION 3.20)
set(CMAKE_C_COMPILER_FORCED TRUE)
set(CMAKE_C_COMPILER "/bin/echo")
set(CMAKE_C_COMPILER_ID "GNU")
project(stampprobe VERSION 0.0.0 LANGUAGES C)
include("{macro}")
add_library(probe STATIC probe.c)
add_cpp_git_information(probe PRIVATE)
"""


def _git(src, *args, env=None):
    subprocess.check_call(["git", *args], cwd=src, env=env)


def _readonly_except_git(src):
    for dirpath, dirnames, filenames in os.walk(src):
        dirnames[:] = [name for name in dirnames if name != ".git"]
        os.chmod(dirpath, stat.S_IRUSR | stat.S_IXUSR | stat.S_IRGRP | stat.S_IXGRP)
        for name in filenames:
            os.chmod(
                os.path.join(dirpath, name),
                stat.S_IRUSR | stat.S_IRGRP,
            )


def _writable(src):
    for dirpath, dirnames, filenames in os.walk(src):
        os.chmod(dirpath, stat.S_IRWXU)
        for name in filenames:
            os.chmod(os.path.join(dirpath, name), stat.S_IRUSR | stat.S_IWUSR)


def _init_repo(src, env):
    """One annotated tag so `git describe` succeeds. Lightweight tags do not."""
    _git(src, "init", "-b", "main")
    _git(src, "config", "user.email", "stamp@example.com")
    _git(src, "config", "user.name", "stamp")
    _git(src, "add", ".")
    _git(src, "commit", "-m", "init", env=env)
    _git(src, "tag", "-a", "v0.0.0", "-m", "v0.0.0", env=env)


def test_read_only_source_configure_writes_the_build_stamp(tmp_path):
    """Working git and a read-only checkout configure without touching sources."""
    src = tmp_path / "src"
    (src / "cmake").mkdir(parents=True)
    (src / "probe.c").write_text("int probe(void) { return 0; }\n", encoding="utf-8")
    (src / "cmake" / "NrnGitInfo.cmake").write_text(
        GIT_INFO.read_text(encoding="utf-8"),
        encoding="utf-8",
    )
    (src / "CMakeLists.txt").write_text(
        LISTS.format(macro=MACRO_HELPER.as_posix()),
        encoding="utf-8",
    )
    env = os.environ.copy()
    env["GIT_AUTHOR_NAME"] = "stamp"
    env["GIT_AUTHOR_EMAIL"] = "stamp@example.com"
    env["GIT_COMMITTER_NAME"] = "stamp"
    env["GIT_COMMITTER_EMAIL"] = "stamp@example.com"
    _init_repo(src, env)
    _readonly_except_git(src)
    build = tmp_path / "b"
    try:
        subprocess.check_call(
            ["cmake", "-S", str(src), "-B", str(build), "-G", "Ninja"],
            cwd=tmp_path,
        )
        assert (build / "nrn-git-info.cmake").is_file()
        assert not (src / "cmake" / "nrn-git-info.cmake").exists()
        text = (build / "nrn-git-info.cmake").read_text(encoding="utf-8")
        assert "GIT_DESCRIBE" in text
    finally:
        _writable(src)


def test_script_mode_still_writes_the_source_stamp(tmp_path):
    """cmake -P keeps the guest recipe that stamps the source tree."""
    src = tmp_path / "src"
    cmake_dir = src / "cmake"
    cmake_dir.mkdir(parents=True)
    (cmake_dir / "NrnGitInfo.cmake").write_text(
        GIT_INFO.read_text(encoding="utf-8"),
        encoding="utf-8",
    )
    (src / "README").write_text("stamp\n", encoding="utf-8")
    env = os.environ.copy()
    env["GIT_AUTHOR_NAME"] = "stamp"
    env["GIT_AUTHOR_EMAIL"] = "stamp@example.com"
    env["GIT_COMMITTER_NAME"] = "stamp"
    env["GIT_COMMITTER_EMAIL"] = "stamp@example.com"
    _init_repo(src, env)
    subprocess.check_call(
        ["cmake", "-P", str(cmake_dir / "NrnGitInfo.cmake")],
        cwd=src,
    )
    stamp = cmake_dir / "nrn-git-info.cmake"
    assert stamp.is_file()
    assert "GIT_DESCRIBE" in stamp.read_text(encoding="utf-8")
