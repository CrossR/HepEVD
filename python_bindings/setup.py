# Top level setup.py for the python bindings

# Prior to Python 3.12 distutils was the standard way to build python extensions
# distutils is now deprecated in favor of setuptools

try:
    from setuptools import setup, Extension
except ImportError:
    from distutils.core import setup, Extension

include_dirs = ["../"]
define_macros = []

try:
    import numpy
    print("Numpy is installed, bindings will be built with numpy support.")
    include_dirs.append(numpy.get_include())

    # Define that we are using the Numpy API
    define_macros.append(("USE_NUMPY", 1))

    # Numpy 1.7 deprecated the NPY_NO_DEPRECATED_API macro
    define_macros.append(("NPY_NO_DEPRECATED_API", "NPY_1_7_API_VERSION"))
except ImportError:
    print("Numpy is missing, bindings will be built without numpy support.")

hep_evd =  Extension(
    'hep_evd',
    sources = ['hep_evd_bindings.cpp'],
    include_dirs=include_dirs,
    language='c++',
    extra_compile_args=['-std=c++17', '-O3'],
    define_macros=define_macros,
)
setup (ext_modules=[hep_evd], name='hep_evd', version='1.0')