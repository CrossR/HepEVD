import os
import sys

def print_dir_structure(path, prefix="", is_last=True, max_depth=3, current_depth=0):
    if current_depth > max_depth:
        return
        
    if not os.path.exists(path):
        print(f"{prefix}{'└── ' if is_last else '├── '}{os.path.basename(path)} (NOT FOUND)")
        return
        
    print(f"{prefix}{'└── ' if is_last else '├── '}{os.path.basename(path)}")
    
    if os.path.isdir(path):
        files = sorted(os.listdir(path))
        for i, file in enumerate(files):
            new_prefix = prefix + ("    " if is_last else "│   ")
            print_dir_structure(
                os.path.join(path, file), 
                new_prefix, 
                i == len(files) - 1,
                max_depth,
                current_depth + 1
            )

# Try to find the site-packages directory
site_packages = None
for p in sys.path:
    if 'site-packages' in p:
        site_packages = p
        break

if site_packages:
    print(f"Site-packages directory: {site_packages}")
    print("\nHepEVD package structure:")
    print_dir_structure(os.path.join(site_packages, 'HepEVD'))
else:
    print("Could not find site-packages directory")

# Try to import HepEVD and check where module files are located
try:
    import importlib.util
    import importlib
    
    # Check if we can find the package
    hepevd_spec = importlib.util.find_spec('HepEVD')
    if hepevd_spec:
        print(f"\nHepEVD found at: {hepevd_spec.origin}")
        
        # Try to get the module directly
        try:
            init_module = importlib.util.module_from_spec(hepevd_spec)
            hepevd_spec.loader.exec_module(init_module)
            print("Successfully loaded HepEVD module directly")
        except Exception as e:
            print(f"Error loading HepEVD directly: {e}")
    else:
        print("HepEVD package not found")
        
    # Check for the extension module
    ext_spec = importlib.util.find_spec('HepEVD._hepevd_impl')
    if ext_spec:
        print(f"\n_hepevd_impl found at: {ext_spec.origin}")
    else:
        print("\n_hepevd_impl not found")
except Exception as e:
    print(f"Error during import tests: {e}")

import os
import sys
import importlib.util

# Try to find HepEVD
for p in sys.path:
    init_path = os.path.join(p, 'HepEVD', '__init__.py')
    if os.path.exists(init_path):
        print(f"Found __init__.py at: {init_path}")
        
        # Check file contents
        with open(init_path, 'r') as f:
            content = f.read()
            print("\nFirst 200 characters of file:")
            print(content[:200])
            print("...")
            
            # Check for specific patterns
            if "for name in dir(_hepevd_impl)" in content:
                print("\nFile contains the expected code for importing symbols")
            else:
                print("\nFile does NOT contain the expected code for importing symbols")