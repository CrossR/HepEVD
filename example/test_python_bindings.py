import random

import hep_evd

# Test Python Bindings
print("Testing Python Bindings")

# Check if initialised
print(f"Checking if initialised: {hep_evd.is_init()}")

# Initialise
print("Initialising...")
detector_geometry = [
    ((-182.954544067, 0, 696.293762207), 359.415008545, 1207.84753418, 1394.33996582),
    ((182.954544067, 0, 696.293762207), 359.415008545, 1207.84753418, 1394.33996582),
]
hep_evd.set_geo(detector_geometry)

# Should now be initialised
print(f"Checking if initialised: {hep_evd.is_init()}")

print("Add some hits...")
hits = [
    [
        [
            random.uniform(-350, 350),
            random.uniform(-600, 600),
            random.uniform(0, 1300),
        ],
        random.uniform(0, 25),
    ]
    for _ in range(25000)
]
hep_evd.add_hits(hits)

print("Save the current state...")
hep_evd.set_verbose(True)
hep_evd.save_state("First")

print("Add some more hits...")
left_hits = [hit for hit in hits if hit[0][0] < 0]
hep_evd.add_hits(left_hits)
hep_evd.save_state("Second")

print("Adding even more hits...")
right_hits = [hit for hit in hits if hit[0][0] > 0]
hep_evd.add_hits(right_hits)
hep_evd.save_state("Third")

print("Testing event display")
hep_evd.start_server()

print("Finished testing Python Bindings!")
