# Top level setup.py for the python bindings

# Prior to Python 3.12 distutils was the standard way to build python extensions
# distutils is now deprecated in favor of setuptools

import os

try:
    from setuptools import setup, Extension, find_packages
except ImportError:
    from distutils.core import setup, Extension, find_packages

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

# Get the absolute path to the ../web directory,
# so it can be used regardless of where HepEVD is installed.
# TODO: Install the web directory in the package.
web_dir = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", "web"))

hep_evd = None
extension_kwargs = {
    "include_dirs": include_dirs,
    "language": "c++",
    "extra_compile_args": [
        f'-DHEP_EVD_WEB_DIR="{web_dir}"',
        "-std=c++17",
        "-O3",
        "-Wno-write-strings",
        "-Wno-unused-function", # There is likely some functions the Python bindings do not use.
    ],
    "define_macros": define_macros,
}

hep_evd = Extension(
    "hep_evd", sources=["basic_hep_evd_bindings.cpp"], **extension_kwargs
)

setup(
    name="hep_evd",
    version="1.0",
    ext_modules=[hep_evd],
    include_package_data=True,
    packages=find_packages(where="data"),
    package_dir={"": "data"},
)
