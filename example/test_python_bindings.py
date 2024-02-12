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

print("Set geometry with objects...")
hep_evd.set_geo(geometry=detector_geometry)

# Should now be initialised, then reset and set geometry with name
print(f"Checking if initialised: {hep_evd.is_init()}")
hep_evd.reset_server(True)
print(f"Checking if initialised: {hep_evd.is_init()}")

print("Set geo with name...")
hep_evd.set_geo(name="dunefd_1x2x6")

# Should now be initialised
print(f"Checking if initialised: {hep_evd.is_init()}")

print("Add some hits...")
threeD_hits = [
    [
        random.uniform(-350, 350),
        random.uniform(-600, 600),
        random.uniform(0, 1300),
    ]
    for _ in range(25000)
]

# Update the hits to include an energy, which is just the sum of the coordinates
threeD_hits = [ [*hit, sum(hit)] for hit in threeD_hits ]

views = [hep_evd.HIT_TYPE.TWO_D_U, hep_evd.HIT_TYPE.TWO_D_V, hep_evd.HIT_TYPE.TWO_D_W]
twoD_hits = [
    [
        random.uniform(-350, 350),
        0.0,
        random.uniform(0, 1300),
        random.uniform(0, 25),
        hep_evd.HIT_DIM.TWO_D,
        view,
    ]
    for _ in range(5000) for view in views
]

hep_evd.add_hits(threeD_hits)
hep_evd.add_hits(twoD_hits)

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

    hep_evd.add_hit_props(hit, properties)

print("Save the current state...")
hep_evd.set_verbose(True)
hep_evd.save_state("First")

print("Add some more hits...")
left_hits = [hit for hit in threeD_hits if hit[0] < 0]
hep_evd.add_hits(left_hits)
hep_evd.save_state("Second")

print("Adding even more hits...")
right_hits = [hit for hit in threeD_hits if hit[0] > 0]
hep_evd.add_hits(right_hits)
hep_evd.save_state("Third")

print("Testing event display")
hep_evd.start_server()

print("Finished testing Python Bindings!")
