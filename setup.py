import os
import re
import sys
import subprocess
from distutils.version import LooseVersion
from setuptools import Command, Extension
from setuptools import setup, find_packages
from setuptools.command.build_ext import build_ext
from platform import python_version
from glob import glob

RX3D = True

if RX3D:
    from Cython.Distutils import Extension as CyExtension
    from Cython.Distutils import build_ext
    import numpy


# Main source of the version. Dont rename
__version__ = '7.8.11'


class CMakeAugmentedExtension(Extension):
    def __init__(self,
                 name,
                 sources,
                 cmake_proj_dir="",
                 cmake_install_prefix="_install",
                 cmake_ld_path = None,
                 cmake_flags=None,
                 cmake_collect_dirs=None,
                 **kw):
        Extension.__init__(self, name, sources, **kw)
        self.sourcedir = os.path.abspath(cmake_proj_dir)
        self.cmake_flags = cmake_flags or []
        self.cmake_install_prefix = cmake_install_prefix
        self.cmake_collect_dirs = cmake_collect_dirs or []


class CMakeAugmentedBuilder(build_ext):
    def run(self, *args, **kw):
        print("CWD: " + os.getcwd())
        for ext in self.extensions:
            if isinstance(ext, CMakeAugmentedExtension):
                self.run_cmake(ext)
                # AAdd the temp include paths
                ext.include_dirs += [os.path.join(self.build_temp, inc_dir)
                                     for inc_dir in ext.include_dirs
                                     if not os.path.isabs(inc_dir)]

                # Collect project files to be installed
                data_files = getattr(self.distribution, 'data_files')
                if data_files is None:
                    data_files = self.distribution.data_files = []
                for d in ext.cmake_collect_dirs:
                    cdir = os.path.join(ext.cmake_install_prefix, d)
                    cfiles = [os.path.join(cdir, f)
                              for f in os.listdir(cdir)
                              if os.path.isfile(os.path.join(cdir, f))]
                    data_files.append((d, cfiles))
                print("Done building CMake project. Now building python extension")

        build_ext.run(self, *args, **kw)


    def run_cmake(self, ext):
        cmake = self._find_cmake()
        self.outdir = os.path.abspath(ext.cmake_install_prefix)
        print("Building lib to:", self.outdir)

        cmake_args = [
            # Generic options only. project options shall be passed as ext param
            '-DCMAKE_INSTALL_PREFIX=' + self.outdir,
            '-DPYTHON_EXECUTABLE=' + sys.executable,
        ] + ext.cmake_flags

        cfg = 'Debug' if self.debug else 'Release'
        cmake_args += ['-DCMAKE_BUILD_TYPE=' + cfg]
        build_args = ['--config', cfg, '--', '-j4']

        env = os.environ.copy()
        env['CXXFLAGS'] = "{} -DVERSION_INFO='{}'".format(env.get('CXXFLAGS', ''),
                                                          self.distribution.get_version())
        if not os.path.exists(self.build_temp):
            os.makedirs(self.build_temp)
        try:
            subprocess.Popen(
                "echo $CXX", shell=True, stdout=subprocess.PIPE)
            subprocess.check_call([cmake, ext.sourcedir] + cmake_args,
                                  cwd=self.build_temp, env=env)
            subprocess.check_call([cmake, '--build', '.', '--target', 'install'] + build_args,
                                  cwd=self.build_temp)
        except subprocess.CalledProcessError as exc:
            print("Status : FAIL", exc.returncode, exc.output)
            raise

    @staticmethod
    def _find_cmake():
        for candidate in ['cmake', 'cmake3']:
            try:
                out = subprocess.check_output([candidate, '--version'])
                cmake_version = LooseVersion(
                    re.search(r'version\s*([\d.]+)', out.decode()).group(1))
                if cmake_version >= '3.5.0':
                    return candidate
            except OSError:
                pass

        raise RuntimeError("Project requires CMake >=3.5.0")


class Docs(Command):
    description = "Generate & optionally upload documentation to docs server"
    user_options = [("upload", None, "Upload to docs server")]
    finalize_options = lambda self: None

    def initialize_options(self):
        self.upload = False

    def run(self):
        # The extensions must be created inplace to inspect docs
        self.reinitialize_command('build_ext', inplace=1)
        self.run_command('build_ext')
        self.run_command('build_sphinx')
        if self.upload:
            self._upload()

    def _upload(self):
        pass


def setup_package():
    docs_require = []  # sphinx, themes, etc
    maybe_docs = docs_require if "docs" in sys.argv else []
    maybe_test_runner = ['pytest-runner'] if "test" in sys.argv else []

    py_packages=[
        'neuron',
        'neuron.neuroml',
        'neuron.tests',
        'neuron.rxd',
        'neuron.crxd',
        'neuron.gui2'
    ] + ["neuron.rxd.geometry3d"] if RX3D else []

    neuron_root = "_install" + python_version().replace('.', '')

    extension_common_params = dict(
        library_dirs = [neuron_root + "/lib"],
        runtime_library_dirs = [],  # Neuron libraries get copied
        extra_link_args = [
            "-Wl,-rpath,{}".format("@loader_path/../../../")
        ] if sys.platform[:6] == "darwin" else [
            "-Wl,-rpath,{}".format("$ORIGIN/../../../")
        ],
        libraries = ["nrniv"]  # "nrnpython{}".format(sys.version_info[0]), "nrniv"],
    )

    extensions = [CMakeAugmentedExtension(
        "neuron.hoc",
        ["src/nrnpython/inithoc.cpp"],
        cmake_install_prefix = neuron_root,
        cmake_ld_path = "lib",
        cmake_collect_dirs = ['bin', 'lib', 'include', 'share/nrn/lib/hoc', 'share/nrn/lib'],
        cmake_flags = [
            '-DNRN_ENABLE_CORENEURON=OFF',
            '-DNRN_ENABLE_INTERVIEWS=OFF',
            '-DNRN_ENABLE_RX3D=OFF',
            '-DNRN_ENABLE_PYTHON_DYNAMIC=OFF',
            '-DNRN_ENABLE_MPI=OFF',
            '-DLINK_AGAINST_PYTHON=OFF',
            '-DNRN_ENABLE_BINARY=OFF',
        ],
        include_dirs = [
            neuron_root + "/include",
            "src",
            "src/oc",
            "src/nrnpython",
            "src/nrnmpi",
        ],
        define_macros = "",
        **extension_common_params
    )]

    if RX3D:
        include_dirs = [
            "share/lib/python/neuron/rxd/geometry3d",
            numpy.get_include()
        ]
        extension_common_params['libraries'].append("rxdmath")
        # cython generated files take too long O3, use O0 for testing
        extension_common_params['extra_compile_args'] = ["-O0"]

        extensions += [
            CyExtension("neuron.rxd.geometry3d.graphicsPrimitives", [
                    "share/lib/python/neuron/rxd/geometry3d/graphicsPrimitives.pyx"
                ],
                **extension_common_params
            ),
            CyExtension("neuron.rxd.geometry3d.ctng", [
                    "share/lib/python/neuron/rxd/geometry3d/ctng.pyx"
                ],
                include_dirs=include_dirs,
                **extension_common_params
            ),
            CyExtension("neuron.rxd.geometry3d.surfaces", [
                    "share/lib/python/neuron/rxd/geometry3d/surfaces.pyx",
                    "src/nrnpython/rxd_marching_cubes.c",
                    "src/nrnpython/rxd_llgramarea.c"
                ],
                include_dirs=include_dirs,
                **extension_common_params
            )
        ]

    setup(
        name='NEURON',
        version=__version__,
        package_dir={'': 'share/lib/python'},
        packages=py_packages,
        ext_modules=extensions,
        cmdclass=dict(build_ext=CMakeAugmentedBuilder, docs=Docs),
        include_package_data=True,
        install_requires=['numpy>=1.13.1'],
        tests_require=["flake8", "pytest"],
        setup_requires=maybe_docs + maybe_test_runner,
        dependency_links=[]
    )


if __name__ == "__main__":
    setup_package()
