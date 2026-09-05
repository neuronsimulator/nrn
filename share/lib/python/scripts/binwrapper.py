#!/usr/bin/env python
"""
A generic wrapper to access nrn binaries from a python installation
Please create a softlink with the binary name to be called.
"""
import os
import platform
import shutil
import subprocess
import sys
import warnings
from importlib.metadata import metadata, PackageNotFoundError
from importlib.util import find_spec
from pathlib import Path

from setuptools.command.build_ext import new_compiler
from packaging.version import Version
from sysconfig import get_config_vars, get_config_var
from find_libpython import find_libpython


def _customize_compiler(compiler):
    """Do platform-specific customizations of compilers on unix platforms."""
    if compiler.compiler_type == "unix":
        (cc, cxx, cflags) = get_config_vars("CC", "CXX", "CFLAGS")
        if "CC" in os.environ:
            cc = os.environ["CC"]
        if "CXX" in os.environ:
            cxx = os.environ["CXX"]
        if "CFLAGS" in os.environ:
            cflags = cflags + " " + os.environ["CFLAGS"]
        cc_cmd = cc + " " + cflags
        # We update executables in compiler to take advantage of distutils arg splitting
        compiler.set_executables(compiler=cc_cmd, compiler_cxx=cxx)


def _set_default_compiler():
    """Set (dont overwrite) CC/CXX so that apps dont use the build-time ones"""
    ccompiler = new_compiler()
    _customize_compiler(ccompiler)
    # UnixCCompiler has .compiler / .compiler_cxx lists. MSVCCompiler does not.
    compiler = getattr(ccompiler, "compiler", None)
    compiler_cxx = getattr(ccompiler, "compiler_cxx", None)
    # xcrun wrapper must bring all args (Mac)
    if compiler and compiler[0] == "xcrun":
        ccompiler.compiler[0] = get_config_var("CC")
        ccompiler.compiler_cxx[0] = get_config_var("CXX")
        compiler = ccompiler.compiler
        compiler_cxx = ccompiler.compiler_cxx
    if compiler:
        os.environ.setdefault("CC", compiler[0])
    if compiler_cxx:
        os.environ.setdefault("CXX", compiler_cxx[0])


def _check_cpp_compiler_version(min_version: str):
    """Check if GCC compiler is >= min supported one, otherwise show warning"""
    try:
        cpp_compiler = os.environ.get("CXX", "")
        version = subprocess.run(
            [cpp_compiler, "--version"],
            stdout=subprocess.PIPE,
        ).stdout.decode("utf-8")
        if "gcc" in version.lower() or "gnu" in version.lower():
            version = subprocess.run(
                [cpp_compiler, "-dumpversion"],
                stdout=subprocess.PIPE,
            ).stdout.decode("utf-8")
            if Version(version) <= Version(min_version):
                warnings.warn(
                    f"Warning: GCC >= {min_version} is required with this version of NEURON"
                    f"but found version {version}",
                )
    except:
        pass


def _config_exe(exe_name):
    """Sets the environment to run the real executable (returned)"""
    try:
        metadata("neuron-nightly")
        print("INFO : Using neuron-nightly Package (Developer Version)")
    except PackageNotFoundError:
        pass

    NRN_PREFIX = str(Path(find_spec("neuron").origin).parent / ".data")

    os.environ["NEURONHOME"] = os.path.join(NRN_PREFIX, "share/nrn")
    os.environ["NRNHOME"] = NRN_PREFIX
    os.environ["CORENRNHOME"] = NRN_PREFIX
    # nrniv skips bash nrnpyenv.sh only when all three NRN_PY* are set.
    os.environ["NRN_PYTHONEXE"] = sys.executable
    os.environ["CORENRN_PYTHONEXE"] = sys.executable
    os.environ["NRN_PYTHONVERSION"] = "{}.{}".format(*sys.version_info[:2])
    os.environ["NRNBIN"] = os.path.dirname(__file__)

    if "NMODLHOME" not in os.environ:
        os.environ["NMODLHOME"] = NRN_PREFIX
    pylib = os.environ.get("NMODL_PYLIB") or find_libpython()
    if not pylib:
        raise ValueError(
            "unable to locate the Python shared library; "
            "please make sure it is installed, "
            "or set the environmental variable `NMODL_PYLIB` "
            "manually to the path to the Python shared library"
        )
    os.environ["NMODL_PYLIB"] = pylib
    os.environ["NRN_PYLIB"] = pylib

    # nmodl module is inside <prefix>/lib directory
    sys.path.insert(0, os.path.join(NRN_PREFIX, "lib"))
    os.environ["PYTHONPATH"] = os.pathsep.join(sys.path)

    bindir = os.path.join(NRN_PREFIX, "bin")
    os.environ["PATH"] = bindir + os.pathsep + os.environ.get("PATH", "")

    _set_default_compiler()
    exe = os.path.join(bindir, exe_name)
    if (
        os.name == "nt"
        and not exe.lower().endswith(".exe")
        and os.path.isfile(exe + ".exe")
    ):
        exe = exe + ".exe"
    return exe


def _wrap_executable(output_name):
    """Create a wrapper for an executable in same dir. Requires renaming the original file.
    Executables are typically found under arch_name
    """
    release_dir = os.path.join(os.environ["NEURONHOME"], "demo/release")
    arch_name = next(os.walk(release_dir))[1][0]  # first dir
    file_path = os.path.join(arch_name, output_name)
    shutil.move(file_path, file_path + ".nrn")
    shutil.copy(__file__, file_path)


def _wrapper_stem(argv0):
    """Command name without a Windows launcher suffix."""
    name = os.path.basename(argv0)
    if os.name == "nt":
        lower = name.lower()
        for suffix in (".exe", ".cmd", ".bat"):
            if lower.endswith(suffix):
                return name[: -len(suffix)]
    return name


def _nrnivmodl_help():
    print("Usage: nrnivmodl [options] [mod files or directories with mod files]")
    print("Options:")
    print("  -h, --help                       Show this help message and exit.")
    print(
        "If no MOD files or directories provided then MOD files from current directory are used."
    )


def _collect_mod_files(args):
    if not args:
        mods = sorted(Path(".").glob("*.mod"))
    elif len(args) == 1 and Path(args[0]).is_dir():
        mods = sorted(Path(args[0]).glob("*.mod"))
    else:
        mods = [Path(item) for item in args]
    resolved = []
    for mod in mods:
        if not mod.is_file():
            raise SystemExit(f"nrnivmodl: ERROR: Mod file {mod} does not exist!")
        resolved.append(mod.resolve())
    return resolved


def _nrnivmodl_cmake(args):
    """Build nrnmech via the shipped neuron CMake package (Windows wheel path)."""
    rest = list(args)
    while rest and rest[0].startswith("-"):
        opt = rest.pop(0)
        if opt in ("-h", "--help"):
            _nrnivmodl_help()
            return 0
        if opt == "-coreneuron":
            raise SystemExit(
                "nrnivmodl: -coreneuron is not enabled in this Windows wheel"
            )
        raise SystemExit(f"{opt} unrecognized, check available CLI options with --help")

    print(os.getcwd())
    mods = _collect_mod_files(rest)
    if not mods:
        print("nrnivmodl: no MOD files to compile")
        return 0

    cmake = shutil.which("cmake")
    if not cmake:
        raise SystemExit(
            "nrnivmodl: cmake not found on PATH; install CMake from https://cmake.org/download/"
        )

    if os.name == "nt":
        from neuron._windows_cxx import MSVC_CXX_MISSING, msvc_cl_available

        mingw = (
            Path(os.environ.get("NRNHOME", ""))
            / "mingw"
            / "mingw64"
            / "bin"
            / "x86_64-w64-mingw32-g++.exe"
        )
        if not mingw.is_file() and not msvc_cl_available():
            raise SystemExit(MSVC_CXX_MISSING.strip())

    prefix = Path(os.environ["NRNHOME"])
    srcdir = prefix / "lib" / "cmake" / "neuron" / "nrnivmodl"
    if not (srcdir / "CMakeLists.txt").is_file():
        raise SystemExit(f"nrnivmodl: missing {srcdir / 'CMakeLists.txt'}")

    units = prefix / "share" / "nrn" / "lib" / "nrnunits.lib"
    if units.is_file():
        os.environ.setdefault("MODLUNIT", str(units))

    # Match CMAKE_SYSTEM_PROCESSOR (AMD64 on win_amd64) and Unix uname -m layout.
    builddir = Path.cwd() / platform.machine()
    cmake_cfg = [
        cmake,
        "-S",
        str(srcdir),
        "-B",
        str(builddir),
        f"-DNRNIVMODL_MOD_FILES={';'.join(str(m) for m in mods)}",
        "-DNRNIVMODL_NEURON=ON",
        "-DNRNIVMODL_CORENEURON=OFF",
        "-DNRNIVMODL_SPECIAL=OFF",
        f"-DCMAKE_PREFIX_PATH={prefix}",
    ]
    gen = os.environ.get("CMAKE_GENERATOR", "")
    if os.name == "nt" and "Ninja" not in gen:
        cmake_cfg.extend(["-A", "x64"])

    subprocess.check_call(cmake_cfg)
    build_cmd = [cmake, "--build", str(builddir), "--parallel"]
    if os.name == "nt" and "Ninja" not in gen:
        # neuronTargets-release.cmake; VS defaults to Debug otherwise.
        build_cmd.extend(["--config", "Release"])
    subprocess.check_call(build_cmd)

    if os.name == "nt":
        dest = Path.cwd() / "nrnmech.dll"
        candidates = [
            builddir / "nrnmech.dll",
            builddir / "Release" / "nrnmech.dll",
            builddir / "RelWithDebInfo" / "nrnmech.dll",
            builddir / "Debug" / "nrnmech.dll",
        ]
        for src in candidates:
            if src.is_file():
                if src.resolve() != dest.resolve():
                    shutil.copy2(src, dest)
                print(f"nrnivmodl: {dest}")
                return 0
        raise SystemExit("nrnivmodl: nrnmech.dll was not produced")
    return 0


def _posix_path(path):
    """Forward slashes: InterViews -dll style treats \\n in a Win32 path as newline."""
    return str(path).replace("\\", "/")


def _nrngui(args):
    """Port of bin/nrngui.in for Windows wheels (no bash).

    Unix: nrniv $NEURONHOME/lib/hoc/nrngui.hoc "$@" -
    .data/bin/nrngui is that bash script (Error 193, same as stock
    neurondemo). Hoc path uses forward slashes so InterViews style
    does not treat \\n in share\\nrn as newline.
    Trailing - reads stdin after nrngui.hoc (same as Unix). File argv
    ends like Unix; without -, the GUI would load and exit.
    """
    home = Path(os.environ["NEURONHOME"])
    hoc = home / "lib" / "hoc" / "nrngui.hoc"
    if not hoc.is_file():
        raise SystemExit(f"nrngui: missing {hoc}")
    nrniv = Path(os.environ["NRNHOME"]) / "bin" / "nrniv.exe"
    if not nrniv.is_file():
        raise SystemExit(f"nrngui: not a file: {nrniv}")
    cmd = [str(nrniv), _posix_path(hoc), *args, "-"]
    # GHA pwsh keeps stdin open; a leftover '-' would hang if quit() never ran.
    kwargs = {}
    if not sys.stdin.isatty():
        kwargs["stdin"] = subprocess.DEVNULL
    return subprocess.call(cmd, **kwargs)  # NOSONAR


def _neurondemo(args):
    """Port of bin/neurondemo.in for Windows wheels (no bash).

    CMake skips generate-neurondemo-mechanism-library on WIN32; the demo
    MOD sources ship at share/nrn/demo/release. First run compiles them
    with the same nrnivmodl CMake path as Test 4 (cwd/nrnmech.dll), then
    nrniv -dll that DLL demo.hoc. NRNDEMO is the HOC $(NRNDEMO) prefix
    (trailing slash). Unix appends '-' so stdin is read after demo.hoc;
    Windows does the same (file argv ends like Unix).
    """
    home = Path(os.environ["NEURONHOME"])
    demo = home / "demo"
    release = demo / "release"
    hoc = demo / "demo.hoc"
    if not hoc.is_file():
        raise SystemExit(f"neurondemo: missing {hoc}")
    if not release.is_dir():
        raise SystemExit(f"neurondemo: missing {release}")

    os.environ["NRNDEMO"] = _posix_path(demo) + "/"

    marker = demo / "neuron"
    dll = release / "nrnmech.dll"
    if not marker.is_file():
        saved = Path.cwd()
        try:
            os.chdir(release)
            rc = _nrnivmodl_cmake([])
        finally:
            os.chdir(saved)
        if rc:
            return rc
        if not dll.is_file():
            raise SystemExit(f"neurondemo: nrnivmodl did not produce {dll}")
        marker.write_text("")

    nrniv = Path(os.environ["NRNHOME"]) / "bin" / "nrniv.exe"
    if not nrniv.is_file():
        raise SystemExit(f"neurondemo: not a file: {nrniv}")
    cmd = [str(nrniv), "-dll", _posix_path(dll), _posix_path(hoc), *args, "-"]
    # GHA pwsh keeps stdin open; a leftover '-' would hang if quit() never ran.
    kwargs = {}
    if not sys.stdin.isatty():
        kwargs["stdin"] = subprocess.DEVNULL
    return subprocess.call(cmd, **kwargs)  # NOSONAR


if __name__ == "__main__":
    wrapper_name = _wrapper_stem(sys.argv[0])
    exe = _config_exe(wrapper_name)

    if wrapper_name == "nrngui" and os.name == "nt":
        sys.exit(_nrngui(sys.argv[1:]))

    if wrapper_name == "neurondemo" and os.name == "nt":
        sys.exit(_neurondemo(sys.argv[1:]))

    if wrapper_name.startswith("nrnivmodl"):
        if os.name == "nt":
            if wrapper_name in ("nrnivmodl-core", "nrnivmodl-all-cmake"):
                raise SystemExit("nrnivmodl-core is not enabled in this Windows wheel")
            sys.exit(_nrnivmodl_cmake(sys.argv[1:]))
        # To create a wrapper for special (so it also gets ENV vars) we intercept nrnivmodl
        _check_cpp_compiler_version("10.0")
        subprocess.check_call([exe, *sys.argv[1:]])
        _wrap_executable("special")
        sys.exit(0)

    if exe.endswith("special"):
        exe = os.path.join(
            sys.argv[0] + ".nrn"
        )  # original special is renamed special.nrn

    if os.name == "nt":
        # os.execv is not exec on Windows: it rebuilds a command line with
        # MSVCRT quoting, which splits -c statements that contain spaces
        # (print "hello"). subprocess.call with a list is argv, not a shell
        # (list2cmdline, same as nrnivmodl).
        if not os.path.isfile(exe):
            raise SystemExit(f"neuron wrapper: not a file: {exe}")
        raise SystemExit(subprocess.call([exe, *sys.argv[1:]]))  # NOSONAR
    os.execv(exe, sys.argv)
