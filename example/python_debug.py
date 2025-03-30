import sys
import os
import platform

print(f"Python version: {sys.version}")
print(f"Platform: {platform.platform()}")
print(f"System path:")
for p in sys.path:
    print(f"  - {p}")

try:
    import HepEVD
    print(f"\nHepEVD module location: {HepEVD.__file__}")
    print(f"HepEVD package path: {HepEVD.__path__}")
    print(f"HepEVD contents: {dir(HepEVD)}")
    
    # Check for the extension module
    try:
        import importlib.util
        spec = importlib.util.find_spec("HepEVD._hepevd_impl")
        if spec:
            print(f"\n_hepevd_impl location: {spec.origin}")
            if os.path.exists(spec.origin):
                print("  Extension file exists")
                
                # Try to load it directly
                import importlib
                try:
                    _hepevd_impl = importlib.import_module("HepEVD._hepevd_impl")
                    print(f"  Successfully loaded _hepevd_impl: {dir(_hepevd_impl)}")
                except ImportError as e:
                    print(f"  Failed to load _hepevd_impl directly: {e}")
                    
                    # Check for library dependencies
                    if platform.system() == "Linux":
                        print("\nChecking library dependencies:")
                        import subprocess
                        try:
                            ldd_output = subprocess.check_output(['ldd', spec.origin], 
                                                                text=True)
                            print(ldd_output)
                        except Exception as e:
                            print(f"  Failed to run ldd: {e}")
            else:
                print("  Extension file does not exist!")
        else:
            print("\n_hepevd_impl not found in sys.path")
    except Exception as e:
        print(f"\nError checking for extension: {e}")
        
except ImportError as e:
    print(f"\nFailed to import HepEVD: {e}")