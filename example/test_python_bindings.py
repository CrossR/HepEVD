import hep_evd

# Test Python Bindings
print("Testing Python Bindings")

# Check if initialised
print(f"Checking if initialised: {hep_evd.is_init()}")

# Initialise
print("Initialising...")
detector_geometry = [
    ((-182.954544067, 0, 696.293762207), 359.415008545, 1207.84753418, 1394.33996582),
    ((182.954544067, 0, 696.293762207), 359.415008545, 1207.84753418, 1394.33996582)
]
hep_evd.set_geo(detector_geometry)

# Should now be initialised
print(f"Checking if initialised: {hep_evd.is_init()}")

print("Testing event display")
hep_evd.start_server()

print("Finished testing Python Bindings!")