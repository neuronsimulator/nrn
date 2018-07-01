# run with
# python setup.py build_ext --inplace

from distutils.core import setup
from distutils.extension import Extension
from Cython.Distutils import build_ext

import numpy

setup(
    cmdclass = {'build_ext': build_ext},
    ext_modules = [
                   Extension("graphicsPrimitives",
                             sources=["graphicsPrimitives.pyx"],
                             include_dirs=[numpy.get_include()]),
                   Extension("ctng",
                             sources=["ctng.pyx"],
                             include_dirs=[numpy.get_include()]),
                   Extension("surfaces",
                             sources=["surfaces.pyx", "marching_cubes2.c", "llgramarea.c"],
                             include_dirs=[numpy.get_include()])],
)

