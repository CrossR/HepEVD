from ._hepevd_impl import *

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
import os
import pathlib

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