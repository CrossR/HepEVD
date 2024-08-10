import random
import numpy as np

import HepEVD


def main() -> None:
    # DUNE FD 1x2x6-like geometry
    detector_geometry = [
        [-182.954544067, 0, 696.293762207, 359.415008545, 1207.84753418, 1394.33996582],
        [182.954544067, 0, 696.293762207, 359.415008545, 1207.84753418, 1394.33996582],
    ]
    assert not HepEVD.is_initialised()
    HepEVD.set_geometry(detector_geometry)

    # Should now be initialised, then reset and set geometry with name
    assert HepEVD.is_initialised()
    HepEVD.reset_server(True)
    assert not HepEVD.is_initialised()

    # Also test set_geometry with string, loading the predefined geometry.
    HepEVD.set_geometry("dunefd_1x2x6")
    assert HepEVD.is_initialised()

    # Generate some 3D hits.
    # Stored as a numpy array of numpy arrays.
    num_hits_3D = 25000
    threeD_hits = np.stack(
        (
            np.random.uniform(-350, 350, num_hits_3D),
            np.random.uniform(-600, 600, num_hits_3D),
            np.random.uniform(0, 1300, num_hits_3D),
        ),
        axis=1,
    )

    # Define the energy of the hits as the sum of the coordinates
    hit_sum = np.sum(threeD_hits, axis=1)
    threeD_hits = np.concatenate((threeD_hits, hit_sum[:, np.newaxis]), axis=1)

    # Now add some 2D hits
    # Stored as a list of lists.
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

    # Add the both sets of hits
    HepEVD.add_hits(threeD_hits)
    HepEVD.add_hits(twoD_hits)

    # Add some properties to the 3D hits
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

    # Add some markers
    vtx3D = HepEVD.Point([0, 0, 0])

    vtx2D = HepEVD.Point([0, 0, 0])
    vtx2D.set_hit_type(HepEVD.HitType.TWO_D_U)
    vtx2D.set_dim(HepEVD.HitDimension.TWO_D)

    HepEVD.add_markers([vtx3D, vtx2D])

    # Test the state functionality
    HepEVD.save_state("First")

    # New state with only hits on the left of the detector
    left_hits = [hit for hit in threeD_hits if hit[0] < 0]
    HepEVD.add_hits(left_hits)
    HepEVD.save_state("Second")

    # And only hits on the right of the detector
    right_hits = np.array([[*hit] for hit in threeD_hits if hit[0] > 0])
    HepEVD.add_hits(right_hits)
    HepEVD.save_state("Third")

    # Set some config
    HepEVD.set_config({"hitColour": "white"})
    HepEVD.set_mc_string("\\nu_e \\rightarrow e^- + \\nu_e")

    HepEVD.start_server()
    return


if __name__ == "__main__":
    main()
