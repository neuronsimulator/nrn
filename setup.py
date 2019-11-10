import os
import re
import sys
import subprocess
from distutils.version import LooseVersion
from setuptools import Command, Extension
from setuptools import setup, find_packages
from setuptools.command.build_ext import build_ext


# Main source of the version. Dont rename
__version__ = '7.8'


class CMakeAugmentedExtension(Extension):
    def __init__(self,
                 name,
                 sources,
                 lib_destination,
                 cmake_proj_dir='',
                 cmake_flags=None,
                 **kw):
        Extension.__init__(self, name, sources, **kw)
        self.sourcedir = os.path.abspath(cmake_proj_dir)
        self.cmake_flags = cmake_flags or []
        self._lib_destination = lib_destination


class CMakeAugmentedBuilder(build_ext):
    def run(self, *args, **kw):
        print("CWD: " + os.getcwd())
        for ext in self.extensions:
            if isinstance(ext, CMakeAugmentedExtension):
                    # and not os.path.isdir(ext._lib_destination):
                self.run_cmake(ext)
                ext.include_dirs += [self.build_temp + "/" + inc_dir
                                     for inc_dir in ext.include_dirs
                                     if not os.path.isabs(inc_dir)]
                print(ext.include_dirs)
            build_ext.run(self, *args, **kw)

    def run_cmake(self, ext):
        cmake = self._find_cmake()
        self.outdir = os.path.abspath(ext._lib_destination)
        print("Building lib to:", self.outdir)
        cmake_args = [
            '-DCMAKE_INSTALL_PREFIX=' + self.outdir,
            '-DPYTHON_EXECUTABLE=' + sys.executable
        ] + ext.cmake_flags

        cfg = 'Debug' if self.debug else 'Release'
        cmake_args += ['-DCMAKE_BUILD_TYPE=' + cfg]
        build_args = ['--config', cfg, '--', '-j24']

        env = os.environ.copy()
        env['CXXFLAGS'] = "{} -static-libstdc++ -DVERSION_INFO='{}'".format(
            env.get('CXXFLAGS', ''),
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

    neuron_root = "install"

    setup(
        name='NEURON',
        version=__version__,
        package_dir={'neuron': 'src/nrnpython'},
        packages=['neuron'],
        ext_modules=[
            CMakeAugmentedExtension("neuron.hoc", [
                    "src/nrnpython/inithoc.cpp"
                ],
                lib_destination=neuron_root,
                cmake_flags=[
                    '-DNRN_ENABLE_CORENEURON=OFF',
                    '-DNRN_ENABLE_INTERVIEWS=OFF',
                    '-DNRN_ENABLE_PYTHON_DYNAMIC=ON',
                    '-DLINK_AGAINST_PYTHON=OFF'
                ],
                library_dirs=[neuron_root + "/lib"],
                extra_link_args = [],
                libraries = ["nrnpython{}".format(sys.version_info[0]), "nrniv"],
                include_dirs = [
                    neuron_root + "/include",
                    "src",
                    "src/oc",
                    "src/nrnpython",
                    "src/nrnmpi",
                ],
                define_macros="",
            ),
        ],
        cmdclass=dict(build_ext=CMakeAugmentedBuilder, docs=Docs),
        include_package_data=True,
        install_requires=['numpy>=1.13.1'],
        tests_require=["flake8", "pytest"],
        setup_requires=maybe_docs + maybe_test_runner,
        dependency_links=[]
    )


if __name__ == "__main__":
    setup_package()
