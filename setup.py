import os
import os.path as osp
import re
import shutil

import sys
import platform
import subprocess

from setuptools import setup, Extension
from setuptools.command.build_ext import build_ext
from setuptools.command.install_lib import install_lib
from distutils.version import LooseVersion


class CMakeExtension(Extension):
    def __init__(self, name, sourcedir=''):
        Extension.__init__(self, name, sources=[])
        self.sourcedir = osp.abspath(sourcedir)


class CMakeBuild(build_ext):
    def run(self):
        try:
            out = subprocess.check_output(['cmake', '--version'])
        except OSError:
            raise RuntimeError("CMake must be installed to build the following extensions: " +
                               ", ".join(e.name for e in self.extensions))

        cmake_version = LooseVersion(re.search(r'version\s*([\d.]+)', out.decode()).group(1))
        if cmake_version < '3.1.0':
            raise RuntimeError("CMake >= 3.1.0 is required")

        for ext in self.extensions:
            self.build_extension(ext)

    def build_extension(self, ext):
        extdir = osp.abspath(osp.dirname(self.get_ext_fullpath(ext.name)))
        extdir = osp.join(extdir, ext.name)
        cmake_args = ['-DCMAKE_LIBRARY_OUTPUT_DIRECTORY=' + extdir,
                      '-DPYTHON_EXECUTABLE=' + sys.executable]

        cfg = 'Debug' if self.debug else 'Release'
        build_args = ['--config', cfg]

        if platform.system() == "Windows":
            cmake_args += ['-DCMAKE_LIBRARY_OUTPUT_DIRECTORY_{}={}'.format(cfg.upper(), extdir)]
            if sys.maxsize > 2**32:
                cmake_args += ['-A', 'x64']
            build_args += ['--', '/m']
        else:
            cmake_args += ['-DCMAKE_BUILD_TYPE=' + cfg]
            build_args += ['--', '-j2']

        env = os.environ.copy()
        env['CXXFLAGS'] = '{} -DVERSION_INFO=\\"{}\\"'.format(env.get('CXXFLAGS', ''),
                                                              self.distribution.get_version())
        if not osp.exists(self.build_temp):
            os.makedirs(self.build_temp)
        subprocess.check_call(['cmake', ext.sourcedir] + cmake_args, cwd=self.build_temp, env=env)
        subprocess.check_call(['cmake', '--build', '.', '--target', '_nmodl'] + build_args, cwd=self.build_temp)


setup(
    name='NMODL',
    version='0.1',
    author='Blue Brain Project',
    author_email='bbp-ou-hpc@groupes.epfl.ch',
    description='NEURON Modelling Language Source-to-Source Compiler Framework',
    long_description='',
    packages=['nmodl'],
    ext_modules=[CMakeExtension('nmodl')],
    cmdclass=dict(build_ext=CMakeBuild),
    zip_safe=False,
    setup_requires=['jinja2>=2.10',
                    'PyYAML>=3.13',
                    'sympy>=1.2'
                    ],
    tests_require=['pytest>=4.0.0']
)
