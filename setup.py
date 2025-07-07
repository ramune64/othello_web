from distutils.core import setup, Extension
from Cython.Build import cythonize

ext = Extension("othello_play_cython2", sources=["othello_play_cython2.pyx"])
setup(name="othello_play_cython2", ext_modules=cythonize([ext]))

""" ext = Extension("othello_play_cython", sources=["othello_play_cython.pyx"])
setup(name="othello_play_cython", ext_modules=cythonize([ext])) """
