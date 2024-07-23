# Top level setup.py for the python bindings

# Prior to Python 3.12 distutils was the standard way to build python extensions
# distutils is now deprecated in favor of setuptools

from glob import glob
import os

try:
    from setuptools import setup, Extension, find_packages
except ImportError:
    from distutils.core import setup, Extension, find_packages

include_dirs = ["../"]
define_macros = []

use_pybind11 = False

try:
    import pybind11
    from pybind11.setup_helpers import Pybind11Extension

    print("pybind11 is installed, bindings will be built with pybind11 support.")
    include_dirs.append(pybind11.get_include())
    define_macros.append(("USE_PYBIND11", 1))
    use_pybind11 = True
except ImportError:
    print("pybind11 is missing, bindings will be built without pybind11 support.")
    print(
        "The bindings without pybind11 support are much more basic, but should still work."
    )

try:
    import numpy

    print("Numpy is installed, bindings will be built with numpy support.")
    include_dirs.append(numpy.get_include())

    # Define that we are using the Numpy API
    define_macros.append(("USE_NUMPY", 1))

    # Numpy 1.7 deprecated the NPY_NO_DEPRECATED_API macro
    define_macros.append(("NPY_NO_DEPRECATED_API", "NPY_1_7_API_VERSION"))
except ImportError:
    if not use_pybind11:
        print("Numpy is missing, bindings will be built without numpy support.")
    else:
        print("Numpy is missing, but pybind11 is installed, please install numpy.")
        print("Bindings for PyBind11 require Numpy.")
        exit(1)

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

if not use_pybind11:
    hep_evd = Extension(
        "hep_evd", sources=["basic_hep_evd_bindings.cpp"], **extension_kwargs
    )
else:
    hep_evd = Pybind11Extension(
        "hep_evd", sorted(glob("pybind11/*.cpp")), **extension_kwargs
    )

setup(
    name="hep_evd",
    version="1.0",
    ext_modules=[hep_evd],
    include_package_data=True,
    packages=find_packages(where="data"),
    package_dir={"": "data"},
)
