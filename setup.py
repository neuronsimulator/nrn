import os
import re
import shutil
import subprocess
import sys
from collections import defaultdict
from distutils import log
from distutils.dir_util import copy_tree
from distutils.version import LooseVersion
from setuptools import Command, Extension
from setuptools import setup


class Components:
    RX3D = True
    IV = True
    MPI = True
    CORENRN = False  # still early support
    GPU = False  # still early support


if os.name != "posix":
    raise Exception(
        "Python NEURON distributions are currently only available "
        "for Mac and Linux systems (POSIX)"
    )

# Main source of the version. Dont rename, used by Cmake
try:
    # github actions somehow fails with check_output and python3

    # Official Versioning shall rely on annotated tags (don't use `--tags` or `--all`)
    # (please refer to NEURON SCM documentation)
    v = (
        subprocess.run(["git", "describe"], stdout=subprocess.PIPE)
        .stdout.strip()
        .decode()
    )

    __version__ = v[: v.rfind("-")].replace("-", ".") if "-" in v else v
    # allow to override version during development/testing
    if "NEURON_WHEEL_VERSION" in os.environ:
        __version__ = os.environ["NEURON_WHEEL_VERSION"]

except Exception as e:
    raise RuntimeError("Could not get version from Git repo : " + str(e))


# setup options must be checked for very early as it impacts imports
if "--disable-rx3d" in sys.argv:
    Components.RX3D = False
    sys.argv.remove("--disable-rx3d")

if "--disable-iv" in sys.argv:
    Components.IV = False
    sys.argv.remove("--disable-iv")

if "--disable-mpi" in sys.argv:
    Components.MPI = False
    sys.argv.remove("--disable-mpi")

if "--enable-coreneuron" in sys.argv:
    Components.CORENRN = True
    sys.argv.remove("--enable-coreneuron")

if "--enable-gpu" in sys.argv:
    Components.GPU = True
    sys.argv.remove("--enable-gpu")

if Components.RX3D:
    try:
        from Cython.Distutils import Extension as CyExtension
        from Cython.Distutils import build_ext
        import numpy
    except ImportError:
        log.error(
            "ERROR: RX3D wheel requires Cython and numpy. Please install beforehand"
        )
        sys.exit(1)
else:
    from setuptools.command.build_ext import build_ext


class CMakeAugmentedExtension(Extension):
    """
    Definition of an extension that depends on a project to be built using CMake
    Notice by default the cmake project is installed to build/cmake_install
    """

    def __init__(
        self,
        name,
        sources,
        cmake_proj_dir="",
        cmake_flags=None,
        cmake_collect_dirs=None,
        cmake_install_python_files="lib/python",
        **kw,
    ):
        """Creates a CMakeAugmentedExtension.

        Args:
            name: the full name of the extension module (e.g. neuron.hoc)
            sources: The cpp sources of the extension
            cmake_proj_dir: The location of the cmake project
            cmake_flags: A list of options to be passed to cmake
            cmake_collect_dirs: Upon cmake success, these dirs are copied
                as package data in a dir '.data'
            cmake_install_python_files: Path to be scanned for python
                modules to be added directly to the python package tree
        """
        Extension.__init__(self, name, sources, **kw)
        self.sourcedir = os.path.abspath(cmake_proj_dir)
        self.cmake_flags = cmake_flags or []
        self.cmake_install_prefix = os.path.abspath("build/cmake_install")
        self.cmake_collect_dirs = cmake_collect_dirs or []
        self.cmake_install_python_files = cmake_install_python_files
        self.cmake_done = False


class CMakeAugmentedBuilder(build_ext):
    """Builder which understands CMakeAugmentedExtension"""

    user_options = build_ext.user_options + [
        ("cmake-prefix=", None, "value for CMAKE_PREFIX_PATH"),
        ("cmake-defs=", None, "Additional CMake definitions, comma split"),
    ]
    boolean_options = build_ext.boolean_options + ["docs"]

    def initialize_options(self):
        build_ext.initialize_options(self)
        self.cmake_prefix = None
        self.cmake_defs = None
        self.docs = False

    def run(self, *args, **kw):
        """Execute the extension builder.
        In case the given extension is a CMakeAugmentedExtension launches
        the CMake building, sets the extension build environment and collects files.
        """
        for ext in self.extensions:

            if isinstance(ext, CMakeAugmentedExtension):
                if ext.cmake_done:
                    continue

                self._run_cmake(ext)  # Build cmake project

                if self.docs:
                    return

                # Collect project files to be installed
                # These go directly into final package, regardless of setuptools filters
                log.info("\n==> Collecting CMAKE files")
                rel_package = ext.name.split(".")[:-1]
                package_data_d = os.path.join(
                    self.build_lib, *(rel_package + [".data"])
                )

                # Python files created/configured by CMake
                src_py_dir = os.path.join(
                    ext.cmake_install_prefix, ext.cmake_install_python_files
                )
                if os.path.isdir(src_py_dir):
                    copy_tree(src_py_dir, self.build_lib)  # accepts existing dst dir
                    shutil.rmtree(src_py_dir)  # avoid being collected to data dir

                for d in ext.cmake_collect_dirs:
                    log.info("  - Collecting %s (and everything under it)", d)
                    src_dir = os.path.join(ext.cmake_install_prefix, d)
                    dst_dir = os.path.join(package_data_d, d)
                    if not os.path.isdir(dst_dir):
                        shutil.copytree(src_dir, dst_dir)
                log.info("==> Done building CMake project\n.")

                # Make the temp include paths in the building the extension
                ext.include_dirs += [
                    os.path.join(self.build_temp, inc_dir)
                    for inc_dir in ext.include_dirs
                    if not os.path.isabs(inc_dir)
                ]
                ext.cmake_done = True

        # Now build the extensions normally
        log.info("==> Building Python extensions")
        build_ext.run(self, *args, **kw)

    def _run_cmake(self, ext):
        cmake = self._find_cmake()
        cfg = "Debug" if self.debug else "Release"
        self.outdir = os.path.abspath(ext.cmake_install_prefix)

        log.info("Building lib to: %s", self.outdir)
        cmake_args = [
            # Generic options only. project options shall be passed as ext param
            "-DCMAKE_INSTALL_PREFIX=" + self.outdir,
            "-DPYTHON_EXECUTABLE=" + sys.executable,
            "-DCMAKE_BUILD_TYPE=" + cfg,
        ] + ext.cmake_flags
        # RTD neds quick config
        if self.docs and os.environ.get("READTHEDOCS"):
            cmake_args = ["-DNRN_ENABLE_MPI=OFF", "-DNRN_ENABLE_INTERVIEWS=OFF"]
        if self.docs:
            cmake_args.append("-DNRN_ENABLE_DOCS=ON")
            cmake_args.append("-DNRN_ENABLE_DOCS_WITH_EXTERNAL_INSTALLATION=ON")
        if self.cmake_prefix:
            cmake_args.append("-DCMAKE_PREFIX_PATH=" + self.cmake_prefix)
        if self.cmake_defs:
            cmake_args += ["-D" + opt for opt in self.cmake_defs.split(",")]

        build_args = ["--config", cfg, "--", "-j4"]  # , 'VERBOSE=1']

        env = os.environ.copy()
        env["CXXFLAGS"] = "{} -DVERSION_INFO='{}'".format(
            env.get("CXXFLAGS", ""), self.distribution.get_version()
        )
        if not os.path.exists(self.build_temp):
            os.makedirs(self.build_temp)

        try:
            # Configure project
            subprocess.Popen("echo $CXX", shell=True, stdout=subprocess.PIPE)
            log.info("[CMAKE] cmd: %s", " ".join([cmake, ext.sourcedir] + cmake_args))
            subprocess.check_call(
                [cmake, ext.sourcedir] + cmake_args, cwd=self.build_temp, env=env
            )
            if self.docs:
                # RTD will call sphinx for us. We just need notebooks and doxygen
                if os.environ.get("READTHEDOCS"):
                    subprocess.check_call(
                        ["make", "notebooks"], cwd=self.build_temp, env=env
                    )
                    subprocess.check_call(
                        ["make", "doxygen"], cwd=self.build_temp, env=env
                    )
                else:
                    subprocess.check_call(
                        ["make", "docs"], cwd=self.build_temp, env=env
                    )
            else:
                subprocess.check_call(
                    [cmake, "--build", ".", "--target", "install"] + build_args,
                    cwd=self.build_temp,
                    env=env,
                )
                subprocess.check_call(
                    [
                        ext.cmake_install_prefix + "/bin/neurondemo",
                        "-nopython",
                        "-nogui",
                        "-c",
                        "quit()",
                    ],
                    cwd=self.build_temp,
                    env=env,
                )
                # mac: libnrnmech of neurondemo need to point to relative libnrniv
                REL_RPATH = (
                    "@loader_path" if sys.platform[:6] == "darwin" else "$ORIGIN"
                )
                subprocess.check_call(
                    [
                        ext.sourcedir + "/packaging/python/fix_demo_libnrnmech.sh",
                        ext.cmake_install_prefix,
                        REL_RPATH,
                    ],
                    cwd=self.build_temp,
                    env=env,
                )
                if Components.GPU:
                    subprocess.check_call(
                        [
                            ext.sourcedir
                            + "/packaging/python/fix_target_processor_in_makefiles.sh",
                            ext.cmake_install_prefix,
                        ],
                        cwd=self.build_temp,
                        env=env,
                    )

        except subprocess.CalledProcessError as exc:
            log.error("Status : FAIL. Log:\n%s", exc.output)
            raise

    @staticmethod
    def _find_cmake():
        for candidate in ["cmake", "cmake3"]:
            try:
                out = subprocess.check_output([candidate, "--version"])
                cmake_version = LooseVersion(
                    re.search(r"version\s*([\d.]+)", out.decode()).group(1)
                )
                if cmake_version >= "3.15.0":
                    return candidate
            except OSError:
                pass

        raise RuntimeError("Project requires CMake >=3.15.0")


class Docs(Command):
    description = "Generate & optionally upload documentation to docs server"
    user_options = [("upload", None, "Upload to docs server")]
    finalize_options = lambda self: None

    def initialize_options(self):
        self.upload = False

    def run(self):
        # The extensions must be created inplace to inspect docs
        self.reinitialize_command("build_ext", inplace=1, docs=True)
        self.run_command("build_ext")
        if self.upload:
            self._upload()

    def _upload(self):
        pass


def setup_package():
    NRN_PY_ROOT = "share/lib/python"
    NRN_PY_SCRIPTS = os.path.join(NRN_PY_ROOT, "scripts")
    NRN_COLLECT_DIRS = ["bin", "lib", "include", "share"]

    docs_require = []  # sphinx, themes, etc
    maybe_rxd_reqs = ["numpy", "Cython"] if Components.RX3D else []
    maybe_docs = docs_require if "docs" in sys.argv else []
    maybe_test_runner = ["pytest-runner"] if "test" in sys.argv else []

    py_packages = [
        "neuron",
        "neuron.neuroml",
        "neuron.tests",
        "neuron.rxd",
        "neuron.crxd",
        "neuron.gui2",
    ] + (["neuron.rxd.geometry3d"] if Components.RX3D else [])

    REL_RPATH = "@loader_path" if sys.platform[:6] == "darwin" else "$ORIGIN"

    extension_common_params = defaultdict(
        list,
        library_dirs=["build/cmake_install/lib"],
        libraries=["nrniv", "nrnpython{}".format(sys.version_info[0])],
    )

    extensions = [
        CMakeAugmentedExtension(
            "neuron.hoc",
            ["src/nrnpython/inithoc.cpp"],
            cmake_collect_dirs=NRN_COLLECT_DIRS,
            cmake_flags=[
                "-DNRN_ENABLE_CORENEURON=" + ("ON" if Components.CORENRN else "OFF"),
                "-DNRN_ENABLE_INTERVIEWS=" + ("ON" if Components.IV else "OFF"),
                "-DIV_ENABLE_X11_DYNAMIC=" + ("ON" if Components.IV else "OFF"),
                "-DNRN_ENABLE_RX3D=OFF",  # Never build within CMake
                "-DNRN_ENABLE_MPI=" + ("ON" if Components.MPI else "OFF"),
                "-DNRN_ENABLE_MPI_DYNAMIC=" + ("ON" if Components.MPI else "OFF"),
                "-DNRN_ENABLE_PYTHON_DYNAMIC=ON",
                "-DNRN_ENABLE_MODULE_INSTALL=OFF",
                "-DNRN_ENABLE_REL_RPATH=ON",
                "-DLINK_AGAINST_PYTHON=OFF",
                "-DCMAKE_VERBOSE_MAKEFILE=OFF",
                "-DCORENRN_ENABLE_OPENMP=ON",  # TODO: manylinux portability questions
            ]
            + (
                [
                    "-DCORENRN_ENABLE_GPU=ON",
                    "-DCMAKE_C_COMPILER=nvc",  # use nvc and nvc++ for GPU support
                    "-DCMAKE_CXX_COMPILER=nvc++",
                    "-DCMAKE_CUDA_COMPILER=nvcc",
                ]
                if Components.GPU
                else []
            ),
            include_dirs=[
                "src",
                "src/oc",
                "src/nrnpython",
                "src/nrnmpi",
            ],
            extra_link_args=[
                # use relative rpath to .data/lib
                "-Wl,-rpath,{}".format(REL_RPATH + "/.data/lib/")
            ],
            **extension_common_params,
        )
    ]

    if Components.RX3D:
        include_dirs = ["share/lib/python/neuron/rxd/geometry3d", numpy.get_include()]

        # Cython files take a long time to compile with O2 so default O0
        # But pay the price if uploading distribution
        extra_compile_args = ["-O2" if "NRN_BUILD_FOR_UPLOAD" in os.environ else "-O0"]
        rxd_params = extension_common_params.copy()
        rxd_params["libraries"].append("rxdmath")
        rxd_params.update(
            dict(
                # Cython files take a long time to compile with O2 but this
                # is a distribution...
                extra_compile_args=extra_compile_args,
                extra_link_args=[
                    "-Wl,-rpath,{}".format(REL_RPATH + "/../../.data/lib/")
                ],
            )
        )

        log.info("RX3D compile flags %s" % str(rxd_params))

        extensions += [
            CyExtension(
                "neuron.rxd.geometry3d.graphicsPrimitives",
                ["share/lib/python/neuron/rxd/geometry3d/graphicsPrimitives.pyx"],
                **rxd_params,
            ),
            CyExtension(
                "neuron.rxd.geometry3d.ctng",
                ["share/lib/python/neuron/rxd/geometry3d/ctng.pyx"],
                include_dirs=include_dirs,
                **rxd_params,
            ),
            CyExtension(
                "neuron.rxd.geometry3d.surfaces",
                [
                    "share/lib/python/neuron/rxd/geometry3d/surfaces.pyx",
                    "src/nrnpython/rxd_marching_cubes.cpp",
                    "src/nrnpython/rxd_llgramarea.cpp",
                ],
                include_dirs=include_dirs,
                **rxd_params,
            ),
        ]

    log.info("RX3D is %s", "ENABLED" if Components.RX3D else "DISABLED")

    # package name
    package_name = "NEURON-gpu" if Components.GPU else "NEURON"

    # For CI, we want to build separate wheel with "-nightly" suffix
    package_name += os.environ.get("NEURON_NIGHTLY_TAG", "-nightly")

    # GPU wheels use patchelf to avoid duplicating NVIDIA runtime libraries when
    # using nrnivmodl.
    maybe_patchelf = ["patchelf"] if Components.GPU else []

    setup(
        name=package_name,
        version=__version__,
        package_dir={"": NRN_PY_ROOT},
        packages=py_packages,
        package_data={"neuron": ["*.dat"]},
        ext_modules=extensions,
        scripts=[
            os.path.join(NRN_PY_SCRIPTS, f)
            for f in os.listdir(NRN_PY_SCRIPTS)
            if f[0] != "_"
        ],
        cmdclass=dict(build_ext=CMakeAugmentedBuilder, docs=Docs),
        install_requires=["numpy>=1.9.3"] + maybe_patchelf,
        tests_require=["flake8", "pytest"],
        setup_requires=["wheel"] + maybe_docs + maybe_test_runner + maybe_rxd_reqs,
        dependency_links=[],
    )


def mac_osx_setenv():
    """Set MacOS environment to build high-compat wheels"""
    try:
        # Also ensure wheel package is avail
        import wheel
    except ImportError:
        from setuptools.dist import Distribution

        Distribution().fetch_build_eggs(["wheel"])
    from wheel.macosx_libfile import extract_macosx_min_system_version

    sdk_root = (
        subprocess.check_output(["xcrun", "--sdk", "macosx", "--show-sdk-path"])
        .decode()
        .strip()
    )
    log.info("Setting SDKROOT=%s", sdk_root)
    os.environ["SDKROOT"] = sdk_root

    # Extract the macOS version targeted by the Python framework
    py_osx_framework = extract_macosx_min_system_version(sys.executable)

    def fmt(version):
        return ".".join(str(x) for x in version)

    if "MACOSX_DEPLOYMENT_TARGET" in os.environ:
        # Don't override the value if it is set explicitly, but try and print a
        # helpful message
        explicit_target = tuple(
            int(x) for x in os.environ["MACOSX_DEPLOYMENT_TARGET"].split(".")
        )
        if py_osx_framework is not None and explicit_target > py_osx_framework:
            log.warn(
                "You are building wheels for macOS >={}; this is more "
                "restrictive than your Python framework, which supports "
                ">={}".format(fmt(explicit_target), fmt(py_osx_framework))
            )
    else:
        # Target not set explicitly, set MACOSX_DEPLOYMENT_TARGET to match the
        # Python framework, or 10.9 if the version targeted by the framework
        # cannot be determined
        if py_osx_framework is None:
            py_osx_framework = (10, 9)
        if py_osx_framework > (10, 9):
            log.warn(
                "You are building a wheel with a Python built for macOS >={}. "
                "Your wheel won't run on older versions, consider using an "
                "official Python build from python.org".format(fmt(py_osx_framework))
            )
        macos_target = "%d.%d" % tuple(py_osx_framework[:2])
        log.warn("Setting MACOSX_DEPLOYMENT_TARGET=%s", macos_target)
        os.environ["MACOSX_DEPLOYMENT_TARGET"] = macos_target


if __name__ == "__main__":
    if sys.platform[:6] == "darwin":
        mac_osx_setenv()
    setup_package()
