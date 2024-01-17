#include <Python.h>
#include <signal.h>

#include "hep_evd.h"

// Broad layout is the same as the helper files, where we
// define a static, global server object, and then define
// functions to interact with it. The difference here is
// we have very different inputs compared to Pandora or
// LArSoft.
inline HepEVD::HepEVDServer *hepEVDServer;
inline bool verboseLogging = false;

// Define a signal handler to catch SIGINT, SIGTERM, SIGKILL.
// This is so we can gracefully shutdown the server.
void catch_signals() {
    auto handler = [](int code) {
        if (hepEVDServer != nullptr) {
            std::cout << "HepEVD: Caught signal " << code << ", shutting down." << std::endl;
            hepEVDServer->stopServer();
        }
    };

    signal(SIGINT, handler);
    signal(SIGTERM, handler);
    signal(SIGKILL, handler);
}

// Check if the server is initialised.
bool isInitialised() {
    if (hepEVDServer == nullptr || !hepEVDServer->isInitialised())
        return false;

    return true;
}
static PyObject *py_is_initialised(PyObject *self, PyObject *args) {
    if (!isInitialised())
        Py_RETURN_FALSE;

    Py_RETURN_TRUE;
}

// Start the server.
static PyObject *py_start_server(PyObject *self, PyObject *args) {

    if (!isInitialised()) {
        Py_RETURN_FALSE;
    }

    if (verboseLogging) {
        std::cout << "HepEVD: There are " << hepEVDServer->getHits().size() << " hits." << std::endl;
        std::cout << "HepEVD: There are " << hepEVDServer->getMCHits().size() << " MC hits." << std::endl;
        std::cout << "HepEVD: There are " << hepEVDServer->getParticles().size() << " particles." << std::endl;
        std::cout << "HepEVD: There are " << hepEVDServer->getMarkers().size() << " markers." << std::endl;
    }

    unsigned int startState = -1;
    if (PyArg_ParseTuple(args, "|I", &startState) && startState >= 0)
        hepEVDServer->swapEventState(startState);
    else if (hepEVDServer->getState()->isEmpty())
        hepEVDServer->previousEventState();

    catch_signals();
    hepEVDServer->startServer();

    Py_RETURN_TRUE;
}

// Receive detector geometry and initialise the server.
static PyObject *py_set_geo(PyObject *self, PyObject *args) {

    if (isInitialised()) {
        Py_RETURN_TRUE;
    }

    // TODO: Generalise for other volumes, not just boxes.
    HepEVD::Volumes volumes;

    // First, grab the overall detector geometry volume list...
    PyObject *geometryList;
    if (!PyArg_ParseTuple(args, "O", &geometryList)) {
        std::cout << "HepEVD: Failed to parse arguments." << std::endl;
        Py_RETURN_FALSE;
    }

    // Start iterating over that list...
    PyObject *geoIter = PyObject_GetIter(geometryList);
    if (!geoIter) {
        std::cout << "HepEVD: Failed to get iterator for geometry volumes." << std::endl;
        Py_RETURN_FALSE;
    }

    // While we have volumes, parse them out.
    // That means parsing out in a few steps:
    //  - Get the next volume object iterator.
    //  - Parse out the widths, and the position (as a new object).
    //  - Parse out the positions from that object.
    while (true) {

        PyObject *geoTuple = PyIter_Next(geoIter);
        if (!geoTuple)
            break;

        // Check if we have a tuple, and if it has the right number of elements.
        if (!PyTuple_Check(geoTuple) || PyTuple_Size(geoTuple) != 4) {
            std::cout << "HepEVD: Failed to parse geometry tuple." << std::endl;
            Py_RETURN_FALSE;
        }

        // Parse out the position and dimensions.
        // The volume object (for boxes) looks like this:
        // ([x, y, z], xWidth, yWidth, zWidth)
        // We parse out the position as another object, and the dimensions as doubles.
        PyObject *positionTuple = PyTuple_GetItem(geoTuple, 0);
        double xWidth = PyFloat_AsDouble(PyTuple_GetItem(geoTuple, 1));
        double yWidth = PyFloat_AsDouble(PyTuple_GetItem(geoTuple, 2));
        double zWidth = PyFloat_AsDouble(PyTuple_GetItem(geoTuple, 3));

        // Lets finally parse out the positions.
        double x, y, z;
        if (!PyArg_ParseTuple(positionTuple, "ddd", &x, &y, &z)) {
            std::cout << "HepEVD: Failed to parse position tuple." << std::endl;
            Py_RETURN_FALSE;
        }

        volumes.push_back(HepEVD::BoxVolume(HepEVD::Position({x, y, z}), xWidth, yWidth, zWidth));
    }

    // Finally, initialise the server.
    hepEVDServer = new HepEVD::HepEVDServer(HepEVD::DetectorGeometry(volumes));

    Py_RETURN_TRUE;
}

// Basic bindings to run the example from the C++ code.
static PyObject *py_run_example(PyObject *self, PyObject *args) {

    using namespace HepEVD;

    BoxVolume volume1(Position({-182.954544067, 0, 696.293762207}), 359.415008545, 1207.84753418, 1394.33996582);
    BoxVolume volume2(Position({182.954544067, 0, 696.293762207}), 359.415008545, 1207.84753418, 1394.33996582);
    Volumes vols({volume1, volume2});

    Hits hits;
    Hits eventTwoHits;
    Hits eventThreeHits;
    MCHits mcHits;

    std::random_device rd;
    std::mt19937 gen(rd());

    std::uniform_real_distribution<float> disX(-350, 350);
    std::uniform_real_distribution<float> disY(-600, 600);
    std::uniform_real_distribution<float> disZ(0, 1300);

    std::uniform_real_distribution<float> disProb(0, 1);

    const std::vector<double> pdgCodes({11, 13});
    std::uniform_int_distribution<> disPdg(0, 1);

    for (unsigned int i = 0; i < 25000; ++i) {

        const double x = disX(gen);
        const double y = disY(gen);
        const double z = disZ(gen);
        const double e = x + y + z;

        Hit *hit = new Hit({x, y, z}, e);
        MCHit *mcHit = new MCHit({disX(gen), disY(gen), disZ(gen)}, pdgCodes[disPdg(gen)]);

        std::map<std::string, double> properties;

        if (x < -250.0)
            properties["Left"] = 1.0f;
        if (x > 250.0)
            properties["Right"] = 1.0f;

        if (y < -500.0)
            properties["Bottom"] = 1.0f;
        if (y > 500.0)
            properties["Top"] = 1.0f;

        if (z < 100.0)
            properties["Front"] = 1.0f;
        if (z > 1200.0)
            properties["Back"] = 1.0f;

        hit->addProperties(properties);
        hits.push_back(hit);
        mcHits.push_back(mcHit);

        // Setup a second event, where the hits are only those in the left volume
        if (x < 0.0) {
            eventTwoHits.push_back(hit);
        }

        // Setup a third event, where the hits are only those in the right volume
        if (x > 0.0) {
            eventThreeHits.push_back(hit);
        }
    }

    // Repeat for the 2D views
    disX = std::uniform_real_distribution<float>(-350, 350);
    disZ = std::uniform_real_distribution<float>(0, 1300);
    std::array<HitType, 3> views({TWO_D_U, TWO_D_V, TWO_D_W});

    for (unsigned int i = 0; i < 3; ++i) {
        for (unsigned int j = 0; j < 5000; ++j) {
            const double x = disX(gen);
            const double z = disZ(gen);
            const double e = x + z;

            Hit *hit = new Hit({x, 0.f, z}, e);
            hit->setDim(TWO_D);
            hit->setHitType(views[i]);
            hits.push_back(hit);

            MCHit *mcHit = new MCHit({disX(gen), 0.f, disZ(gen)}, pdgCodes[disPdg(gen)]);
            mcHit->setDim(TWO_D);
            mcHit->setHitType(views[i]);
            mcHits.push_back(mcHit);
        }
    }

    HepEVDServer server(DetectorGeometry(vols), hits, mcHits);

    server.addEventState("Second", {}, eventTwoHits, {}, {}, "");
    server.addEventState("Third", {}, eventThreeHits, {}, {}, "");

    server.startServer();

    Py_RETURN_NONE;
}

// Define the methods we want to expose to Python.
static PyMethodDef methods[] = {{"run_example", py_run_example, METH_VARARGS, "Run HepEVD example."},
                                {"is_init", py_is_initialised, METH_VARARGS, "Check if the HepEVD server is initialised."},
                                {"start_server", py_start_server, METH_VARARGS, "Start the HepEVD server."},
                                {"set_geo", py_set_geo, METH_VARARGS, "Set the detector geometry."},
                                {NULL, NULL, 0, NULL}};

// Actually define the module.
static struct PyModuleDef module = {PyModuleDef_HEAD_INIT, "hep_evd", "HepEVD server bindings.", -1, methods};

// And finally, initialise the module.
PyMODINIT_FUNC PyInit_hep_evd(void) { return PyModule_Create(&module); }
