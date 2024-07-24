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
# For now, we ignore and clobber any existing environment variable, since
# it could be confusing or a security risk. Can consider adding a warning
# and relaxing the check in the future.
import os
import pathlib

web_path = (pathlib.Path(__file__).parent / "web").resolve()
os.environ["HEP_EVD_WEB_FOLDER"] = str(web_path)

# Remove the imported module
del pathlib
del os
del web_path