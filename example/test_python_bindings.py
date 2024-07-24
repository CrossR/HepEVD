import random
import numpy as np

import HepEVD

# Test Python Bindings
print("Testing Python Bindings")

# Check if initialised
print(f"Checking if initialised: {HepEVD.is_initialised()}")

# Initialise
print("Initialising...")
detector_geometry = [
    [-182.954544067, 0, 696.293762207, 359.415008545, 1207.84753418, 1394.33996582],
    [182.954544067, 0, 696.293762207, 359.415008545, 1207.84753418, 1394.33996582],
]

print("Set geometry with objects...")
HepEVD.set_geometry(detector_geometry)

# Should now be initialised, then reset and set geometry with name
print(f"Checking if initialised: {HepEVD.is_initialised()}")
HepEVD.reset_server(True)
print(f"Checking if initialised: {HepEVD.is_initialised()}")

print("Set geo with name...")
HepEVD.set_geometry("dunefd_1x2x6")

# Should now be initialised
print(f"Checking if initialised: {HepEVD.is_initialised()}")

print("Add some hits...")
threeD_hits = [
    [random.uniform(-350, 350), random.uniform(-600, 600), random.uniform(0, 1300),]
    for _ in range(25000)
]

# Update the hits to include an energy, which is just the sum of the coordinates
threeD_hits = np.array([[*hit, sum(hit)] for hit in threeD_hits])

views = [HepEVD.HitType.TWO_D_U, HepEVD.HitType.TWO_D_V, HepEVD.HitType.TWO_D_W]
twoD_hits = [
    [
        random.uniform(-350, 350),
        0.0,
        random.uniform(0, 1300),
        random.uniform(0, 25),
        HepEVD.HitDimension.TWO_D,
        view,
    ]
    for _ in range(5000)
    for view in views
]

print("Adding 3D hits from numpy array...")
HepEVD.add_hits(threeD_hits)

print("Adding 2D hits from python list...")
HepEVD.add_hits(twoD_hits)

print("Add propreties to all hits...")
for hit in threeD_hits:
    properties = {}

    if hit[0] < -250:
        properties["Left"] = 1.0
    if hit[0] > 250:
        properties["Right"] = 1.0

    if hit[1] < -500:
        properties["Bottom"] = 1.0
    if hit[1] > 500:
        properties["Top"] = 1.0

    if hit[2] < 100:
        properties["Front"] = 1.0
    if hit[2] > 1200:
        properties["Back"] = 1.0

    HepEVD.add_hit_properties(hit, properties)

print("Save the current state...")
HepEVD.save_state("First")

print("Add some more hits...")
left_hits = [hit for hit in threeD_hits if hit[0] < 0]
HepEVD.add_hits(left_hits)
HepEVD.save_state("Second")

print("Adding even more hits...")
right_hits = np.array([[*hit] for hit in threeD_hits if hit[0] > 0])
HepEVD.add_hits(right_hits)
HepEVD.save_state("Third")

print("Testing event display")
HepEVD.start_server()

print("Finished testing Python Bindings!")
