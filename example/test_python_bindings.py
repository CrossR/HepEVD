import random
import numpy as np

import HepEVD


def main() -> None:
    # Print out the available HepEVD functions.
    # This is useful to see what functions are available, and
    # what their parameters are.
    print("Available HepEVD functions:", dir(HepEVD))

    # Now, lets enable verbose output.
    # This is useful for debugging, and seeing what HepEVD is doing.
    HepEVD.set_verbose(True)

    # There is two ways to pass over a geometry:
    # 1. as a list/array or lists/arrays containing the geometry
    # 2. as a string, with the name of the predefined geometry
    #
    # Here, we define a DUNE FD 1x2x6-like geometry as a list/array.
    # The format is as follows:
    # [x, y, z, dx, dy, dz]
    #
    # This can be either a python list, or numpy-array like objects.
    # For now, a box like geometry is assumed, but could be expanded to
    # cover the other geometry types that are available in HepEVD.
    # These can be seen in `include/geometry.h`.
    detector_geometry = [
        [-182.954544067, 0, 696.293762207, 359.415008545, 1207.84753418, 1394.33996582],
        [182.954544067, 0, 696.293762207, 359.415008545, 1207.84753418, 1394.33996582],
    ]

    # Once defined, we can now set the geometry by passing it over.
    # For testing, I'm asserting that the server is not initialised yet, and then
    # becomes initialised, once the geometry is set.
    assert not HepEVD.is_initialised(quiet=True)
    HepEVD.set_geometry(detector_geometry)
    assert HepEVD.is_initialised(quiet=True)

    # If at any point you want to reset the server, you can call `reset_server`.
    # In most cases, you only want to reset the server, not the geometry. For
    # example, because you are moving to a new event.
    #
    # In the rarer cases where you also need to reset the server, you can also
    # pass `reset_geo=True`.
    assert HepEVD.is_initialised(quiet=True)
    HepEVD.reset_server(reset_geo=True)
    assert not HepEVD.is_initialised(quiet=True)

    # If HepEVD knows the geometry you want, you can instead just pass it as a string.
    # The defined geometries can be found in `python_bindings/src/include/detectors.hpp`.
    assert not HepEVD.is_initialised(quiet=True)
    HepEVD.set_geometry("dunefd_1x2x6")
    assert HepEVD.is_initialised(quiet=True)

    # Now, lets add some 3D and 2D hits.
    # First, just generate a random set of 3D hits.
    num_hits_3D = 25000
    threeD_hits = np.stack(
        (
            np.random.uniform(-350, 350, num_hits_3D),
            np.random.uniform(-600, 600, num_hits_3D),
            np.random.uniform(0, 1300, num_hits_3D),
        ),
        axis=1,
    )

    # We also need an "energy" for each hit, here just the sum of the three
    # components.
    hit_sum = np.sum(threeD_hits, axis=1)

    # Our final 3D hits are a list/array of [x, y, z, energy] values.
    # This format was chosen to be simplest for passing data that is already
    # stored in a HDF5 or ROOT file.
    #
    # If useful, a more dedicated class could be created for this purpose,
    # to more closely mimic the underlying C++ class.
    threeD_hits = np.concatenate((threeD_hits, hit_sum[:, np.newaxis]), axis=1)

    # We then have the exact same for the 2D hits, though this time testing the
    # usage of python lists, instead of numpy arrays.
    #
    # In this case as well, the format is
    # [x, y, z, energy, dimension, view/hitType]
    # with dimension coming from the HepEVD::HitDimension enum, and
    # view/hitType coming from the HepEVD::HitType enum.
    # If using in a non LArTPC environment, the view should be set to
    # HepEVD::HitType::GENERAL.
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

    # We can now finally pass over our two hit lists to HepEVD.
    HepEVD.add_hits(threeD_hits)
    HepEVD.add_hits(twoD_hits)

    # One thing you may want to do is assign properties to your hits.
    # I.e. you have an algorithm that scores a hit based on some
    # criteria, and you want to assign that score to each hit to visualise
    # it in HepEVD.
    #
    # This is achieved using the `add_hit_properties` method, and passing over a
    # dictionary with the properties you want to assign, as well as the
    # handle to the hit.
    # Ideally, this handle should be the same as the one just added
    # in the line above.
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

    # Almost there...
    #
    # We can now add some markers:
    #  - a 2D / 3D Point
    #  - a 2D / 3D Line
    #  - a 2D / 3D Ring
    #
    # Or any combination of these, i.e. many points and lines to
    # visualise a graph.
    #
    # This is simply achieved through the `add_markers` method, by passing over
    # a list of `HepEVD.Point`, `HepEVD.Line` or `HepEVD.Ring` objects.
    vtx3D = HepEVD.Point([0, 0, 0])

    vtx2D = HepEVD.Point([0, 0, 0])
    vtx2D.set_hit_type(HepEVD.HitType.TWO_D_U)
    vtx2D.set_dim(HepEVD.HitDimension.TWO_D)

    HepEVD.add_markers([vtx3D, vtx2D])

    # Finally, there is state management.
    # In some cases, you have a single thing you want to visualise,
    # in which case you can just skip any state code and instead
    # just call `start_server()`.
    #
    # But in lots of cases, you'll have plenty of different things
    # you want to visualise individually, and you want to be able
    # to switch between them.
    #
    # For example, you have a raw 2D input, then some 2D reconstructions, and
    # then a rough 3D reconstruction, and a final full 3D reconstruction.
    #
    # You can uses states to save after each step, and switch between them in
    # the UI later.
    #
    # This is achieved through the `save_state` method, and passing over
    # a string name.
    #
    # Optionally, you can also pass over a `min_size` parameter
    # which will automatically start the server if there is at least `min_size`
    # number of states. This can be useful for showing iterations of things.
    #
    # There is also the `clear_on_show` parameter, which will clear the
    # server after displaying all the states, if used in conjunction with
    # `min_size`.
    HepEVD.save_state("First")

    # Here we define two dummy states, just to show the functionality.
    left_hits = [hit for hit in threeD_hits if hit[0] < 0]
    HepEVD.add_hits(left_hits)
    HepEVD.save_state("Second")

    # And only hits on the right of the detector
    right_hits = np.array([[*hit] for hit in threeD_hits if hit[0] > 0])
    HepEVD.add_hits(right_hits)
    HepEVD.save_state("Third")

    # We can also set some config, such as:
    # - show2D : 1 - Show 2D views
    # - show3D : 1 - Show 3D views
    # - disableMouseOver : 0 - Disable mouse over interactions
    # - hitColour : white - Set the colour of the hits
    # - hitSize : 1.0 - Set the size of the hits
    # - hitOpacity : 1.0 - Set the opacity of the hits
    HepEVD.set_config({"hitColour": "white"})

    # And there is also a persistent MC string in the UI, useful to
    # show event data such as the interaction, or other things
    # like run numbers etc.
    HepEVD.set_mc_string("\\nu_e \\rightarrow e^- + \\nu_e")

    # We can now start the server and view our 3 states.
    #
    # Optionally, you can also pass over a `start_state` parameter
    # which will swap to a given state automatically, though the state
    # can always be changed manually in the UI.
    #
    # You can also pass over `clear_on_show` to reset the server
    # states once the server closes. This is useful for automatically
    # resetting the server once a full event has been shown.
    #
    # Once started, you should get a print out of the server
    # address in the console. You can then view the server in your
    # browser at this address, though if you are remote you may need
    # to use port forwarding.
    HepEVD.start_server()

    # And done! The python code will wait on the above line until the server
    # closes, unless you start messing with running the server in a different
    # process / thread.
    return


if __name__ == "__main__":
    main()
