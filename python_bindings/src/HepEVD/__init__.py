import os
import pathlib
import sys

# Import the C++ extension module and expose all its symbols
try:
    from . import _hepevd_impl

    # Bring all symbols into the HepEVD namespace
    for name in dir(_hepevd_impl):
        if not name.startswith('__'):
            globals()[name] = getattr(_hepevd_impl, name)

    # Keep a reference to the module
    globals()['_hepevd_impl'] = _hepevd_impl

except ImportError as e:
    # Simple error reporting is sufficient now that we know the issue
    print(f"ERROR: Failed to import HepEVD C++ extension: {e}", file=sys.stderr)
    raise

# Set up web folder path for the server
if not os.environ.get("HEP_EVD_WEB_FOLDER"):
    web_path = (pathlib.Path(__file__).parent / "web").resolve()
    os.environ["HEP_EVD_WEB_FOLDER"] = str(web_path)

# Clean up imports
del os
del pathlib
del sys