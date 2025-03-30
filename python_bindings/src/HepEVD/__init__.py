import os
import pathlib
import sys

# Try to import from the C++ extension module
try:
    # First import the module
    from . import _hepevd_impl
    
    # Then explicitly bring all its symbols into the HepEVD namespace
    # This is more robust than "from ._hepevd_impl import *", which
    # was causing issues on Linux.
    for name in dir(_hepevd_impl):
        if not name.startswith('__'):
            globals()[name] = getattr(_hepevd_impl, name)
            
    # Keep a reference to the module
    globals()['_hepevd_impl'] = _hepevd_impl
    
    _IMPORT_ERROR = None
except ImportError as e:
    _IMPORT_ERROR = e
    # Get details about the environment
    print(f"ERROR: Failed to import _hepevd_impl: {e}", file=sys.stderr)
    print(f"Python version: {sys.version}", file=sys.stderr)
    print(f"Python path: {sys.path}", file=sys.stderr)
    print(f"Current directory: {os.getcwd()}", file=sys.stderr)
    print(f"Module location: {__file__}", file=sys.stderr)
    
    # Try to find the extension file
    module_dir = os.path.dirname(__file__)
    for file in os.listdir(module_dir):
        if file.startswith("_hepevd_impl") and (file.endswith(".so") or file.endswith(".pyd")):
            print(f"Found extension file: {os.path.join(module_dir, file)}", file=sys.stderr)
    
    # Re-raise the error to prevent module from loading with missing functionality
    raise

# We need to know the absolute path to the web folder, such that the server
# can open it, to be able to serve the web files.
#
# Doing that at compile time, especially once you consider distribution
# of a compiled package, is a gigantic pain.
#
# Instead, lets transparently set an environment variable that can be
# read at runtime. The server will then know where to look for the web
# files.
#
# Warn if the environment variable is already set, but don't override it.
# This is useful for making basic changes to the web files without having
# to recompile the full package.
if not os.environ.get("HEP_EVD_WEB_FOLDER"):
    web_path = (pathlib.Path(__file__).parent / "web").resolve()
    os.environ["HEP_EVD_WEB_FOLDER"] = str(web_path)
    del web_path
else:
    print(f"WARNING: HEP_EVD_WEB_FOLDER is already set to '{os.environ['HEP_EVD_WEB_FOLDER']}'")
    print("This will not be overridden, but could cause issues, or be loading unexpected code!")

# Remove the imported module
del pathlib
del os
del sys