# Top level setup.py for the python bindings

# Prior to Python 3.12 distutils was the standard way to build python extensions
# distutils is now deprecated in favor of setuptools

try:
    from setuptools import setup, Extension
except ImportError:
    from distutils.core import setup, Extension

hep_evd =  Extension(
    'hep_evd',
    sources = ['hep_evd_bindings.cpp'],
    include_dirs=['../'],
    language='c++',
    extra_compile_args=['-std=c++17'],
)
setup (ext_modules=[hep_evd])