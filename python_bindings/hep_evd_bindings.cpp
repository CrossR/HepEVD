#include <Python.h>
#include <map>
#include <signal.h>

#if USE_NUMPY
#include <numpy/arrayobject.h>
#endif

// Include the HepEVD header files.
#include "hep_evd.h"

// Broad layout is the same as the helper files, where we
// define a static, global server object, and then define
// functions to interact with it. The difference here is
// we have very different inputs compared to Pandora or
// LArSoft.
inline HepEVD::HepEVDServer *hepEVDServer;
inline bool verboseLogging = false;

// Define a basic type to store the full information about a hit.
using PyHit = std::tuple<HepEVD::Position, double, HepEVD::HitDimension, HepEVD::HitType>;
inline std::map<PyHit, HepEVD::Hit *> hitMap;

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
// We've got two versions of this, one for C++ and one for Python.
// The C++ version is used internally, and the Python version is
// used to expose the function to Python.
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

// Toggle verbose logging.
static PyObject *py_verbose_logging(PyObject *self, PyObject *args) {
    bool verbose;
    if (!PyArg_ParseTuple(args, "b", &verbose)) {
        std::cout << "HepEVD: Failed to parse verbose logging arguments." << std::endl;
        Py_RETURN_FALSE;
    }

    verboseLogging = verbose;

    Py_RETURN_TRUE;
}

// Start the server.
// This is the main function that starts the server, and it waits
// until the server is stopped before returning.
static PyObject *py_start_server(PyObject *self, PyObject *args) {

    if (!isInitialised()) {
        Py_RETURN_FALSE;
    }

    // Swap the state back to the previous one if we have an empty state.
    // The current state is likely empty due to a call to save_state.
    if (hepEVDServer->getState()->isEmpty())
        hepEVDServer->previousEventState();

    if (verboseLogging) {
        std::cout << "HepEVD: There are " << hepEVDServer->getHits().size() << " hits." << std::endl;
        std::cout << "HepEVD: There are " << hepEVDServer->getMCHits().size() << " MC hits." << std::endl;
        std::cout << "HepEVD: There are " << hepEVDServer->getParticles().size() << " particles." << std::endl;
        std::cout << "HepEVD: There are " << hepEVDServer->getMarkers().size() << " markers." << std::endl;
    }

    catch_signals();
    hepEVDServer->startServer();

    Py_RETURN_TRUE;
}

// Reset the server.
static PyObject *py_reset_server(PyObject *self, PyObject *args) {

    if (!isInitialised()) {
        Py_RETURN_FALSE;
    }

    hepEVDServer->resetServer();

    Py_RETURN_TRUE;
}

// Custom converter for parsing out hits.
static int HitConverter(PyObject *obj, HepEVD::Hit **result) {

    // Check if we have a tuple, and if it has the right number of elements.
    if (!PyList_Check(obj) || PyList_Size(obj) < 4 || PyList_Size(obj) > 6) {
        std::cout << "HepEVD: Failed to validate hit tuple." << std::endl;
        return 0;
    }

    // Parse out the position and energy.
    double x = PyFloat_AsDouble(PyList_GetItem(obj, 0));
    double y = PyFloat_AsDouble(PyList_GetItem(obj, 1));
    double z = PyFloat_AsDouble(PyList_GetItem(obj, 2));
    double energy = PyFloat_AsDouble(PyList_GetItem(obj, 3));

    // These are both optional, so we need to check if they exist.
    HepEVD::HitDimension dimension = HepEVD::HitDimension::THREE_D;
    if (PyList_Size(obj) >= 5)
        dimension = static_cast<HepEVD::HitDimension>(PyLong_AsLong(PyList_GetItem(obj, 4)));

    HepEVD::HitType hitType = HepEVD::HitType::GENERAL;
    if (PyList_Size(obj) >= 6)
        hitType = static_cast<HepEVD::HitType>(PyLong_AsLong(PyList_GetItem(obj, 5)));

    // Create a new hit, and store it in the result.
    HepEVD::Hit *hit = new HepEVD::Hit({x, y, z}, energy);
    hit->setDim(dimension);
    hit->setHitType(hitType);

    *result = hit;

    return 1;
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

// Add hits to the current server state.
// This function needs to be able to support two different input types:
//  - A list of hits
//  - A numpy array of hits
// In both cases, hits are represented as follows:
//  [x, y, z, energy, dimension?, hitType?]
// With the question marks indicating optional parameters.
static PyObject *py_add_hits(PyObject *self, PyObject *args) {

    if (!isInitialised()) {
        Py_RETURN_FALSE;
    }

    // First, grab the full flat list of hits.
    HepEVD::Hits hits;
    PyObject *hitList;

    if (!PyArg_ParseTuple(args, "O", &hitList)) {
        std::cout << "HepEVD: Failed to parse hit arguments." << std::endl;
        Py_RETURN_FALSE;
    }

    // Start iterating over that list...
    PyObject *hitIter = PyObject_GetIter(hitList);
    if (!hitIter) {
        std::cout << "HepEVD: Failed to get iterator for hits." << std::endl;
        Py_RETURN_FALSE;
    }

    // While we have hits, parse them out.
    // That means parsing out in a few steps:
    //  - Get the next hit object iterator, which is either a list or a numpy array.
    //  - Parse out the hit using the custom converter.
    while (true) {

        PyObject *hitObj = PyIter_Next(hitIter);
        if (!hitObj)
            break;

        HepEVD::Hit *hit;
        if (!HitConverter(hitObj, &hit)) {
            std::cout << "HepEVD: Failed to parse hit." << std::endl;
            Py_RETURN_FALSE;
        }

        hits.push_back(hit);

        // We also need to store the hit in a map, so we can add properties to it later.
        PyHit pyHit = std::make_tuple(hit->getPosition(), hit->getEnergy(), hit->getDim(), hit->getHitType());
        hitMap[pyHit] = hit;
    }

    // Finally, add the hits to the current state.
    hepEVDServer->addHits(hits);

    Py_RETURN_TRUE;
}

// Add properties to a hit.
// This should be easier than surfacing the hit map to Python,
// and then updating it there.
static PyObject *py_add_hit_props(PyObject *self, PyObject *args) {

    if (!isInitialised()) {
        Py_RETURN_FALSE;
    }

    // First, grab the top level hit object.
    PyObject *hitObj;
    PyObject *propsDict;
    if (!PyArg_ParseTuple(args, "OO", &hitObj, &propsDict)) {
        std::cout << "HepEVD: Failed to parse hit property arguments." << std::endl;
        Py_RETURN_FALSE;
    }

    // Parse the hit object into a HepEVD::Hit.
    HepEVD::Hit *inputHit;
    if (!HitConverter(hitObj, &inputHit)) {
        std::cout << "HepEVD: Failed to parse hit for property assignment." << std::endl;
        Py_RETURN_FALSE;
    }

    // Make a PyHit object, and check if we have it in the map.
    PyHit pyHit =
        std::make_tuple(inputHit->getPosition(), inputHit->getEnergy(), inputHit->getDim(), inputHit->getHitType());
    if (hitMap.find(pyHit) == hitMap.end()) {
        std::cout << "HepEVD: Failed to find hit in map." << std::endl;
        Py_RETURN_FALSE;
    }

    // Grab the hit from the map...
    HepEVD::Hit *hit = hitMap[pyHit];

    // Start iterating over that list...
    PyObject *propsIter = PyObject_GetIter(propsDict);
    if (!propsIter) {
        std::cout << "HepEVD: Failed to get iterator for hit properties." << std::endl;
        Py_RETURN_FALSE;
    }

    // While we have properties, parse them out.
    std::map<std::string, double> properties;
    PyObject *key, *value;
    Py_ssize_t pos = 0;
    while (PyDict_Next(propsDict, &pos, &key, &value)) {

        // Parse out the property name and value.
        std::string propName = PyUnicode_AsUTF8(key);
        double propValue = PyFloat_AsDouble(value);

        properties[propName] = propValue;
    }

    // Finally, add the properties to the hit.
    hit->addProperties(properties);

    Py_RETURN_TRUE;
}

// Save the current state, and start a new one.
static PyObject *py_save_state(PyObject *self, PyObject *args) {

    if (!isInitialised()) {
        Py_RETURN_FALSE;
    }

    // There is three parameters:
    //  - The name of the state.
    //  - The minimum size of the current state before starting the server (optional).
    //  - Whether to clear the current state on show (optional).
    char *stateName;
    int minSize = -1;
    bool clearOnShow = true;
    if (!PyArg_ParseTuple(args, "s|ib", &stateName, &minSize, &clearOnShow)) {
        std::cout << "HepEVD: Failed to parse state arguments." << std::endl;
        Py_RETURN_FALSE;
    }

    hepEVDServer->setName(stateName);
    bool shouldIncState = true;

    // If prior to adding the new state, the size of the current state was
    // greater than the minimum size, then start the server.
    //
    // This is useful so you can save states as you go, but start the
    // server after a certain number of states have been saved.
    if (minSize != -1 && hepEVDServer->getNumberOfEventStates() >= minSize) {
        catch_signals();
        hepEVDServer->startServer();

        if (clearOnShow) {
            hepEVDServer->resetServer();
            shouldIncState = false;
        }
    }

    // Finally, add a new state if needed.
    if (shouldIncState) {
        hepEVDServer->addEventState();
        hepEVDServer->nextEventState();
    }

    Py_RETURN_TRUE;
}

// Define the methods we want to expose to Python.
static PyMethodDef methods[] = {
    {"is_init", py_is_initialised, METH_VARARGS, "Check if the HepEVD server is initialised."},
    {"start_server", py_start_server, METH_VARARGS, "Start the HepEVD server."},
    {"reset_server", py_reset_server, METH_VARARGS, "Reset the HepEVD server."},
    {"set_geo", py_set_geo, METH_VARARGS, "Set the detector geometry."},
    {"add_hits", py_add_hits, METH_VARARGS, "Add hits to the current event state."},
    {"add_hit_props", py_add_hit_props, METH_VARARGS, "Add properties to a hit."},
    {"save_state", py_save_state, METH_VARARGS, "Save the current event state."},
    {"set_verbose", py_verbose_logging, METH_VARARGS, "Toggle verbose logging."},
    {NULL, NULL, 0, NULL}};

// Actually define the module.
static struct PyModuleDef module_def = {PyModuleDef_HEAD_INIT, "hep_evd", "HepEVD server bindings.", -1, methods};

// Define submodules for the two enums.
static struct PyModuleDef hit_dimension_def = {PyModuleDef_HEAD_INIT, "HIT_DIM", "HepEVD hit dimensions.", -1, NULL};
static struct PyModuleDef hit_type_def = {PyModuleDef_HEAD_INIT, "HIT_TYPE", "HepEVD hit types.", -1, NULL};

// And finally, initialise the module.
PyMODINIT_FUNC PyInit_hep_evd(void) {

    PyObject *module = PyModule_Create(&module_def);

    // Add the enum values to their respective modules.
    PyObject *hit_dimension_module = PyModule_Create(&hit_dimension_def);
    PyModule_AddIntConstant(hit_dimension_module, "THREE_D", HepEVD::THREE_D);
    PyModule_AddIntConstant(hit_dimension_module, "TWO_D", HepEVD::TWO_D);

    PyObject *hit_type_module = PyModule_Create(&hit_type_def);
    PyModule_AddIntConstant(hit_type_module, "GENERAL", HepEVD::GENERAL);
    PyModule_AddIntConstant(hit_type_module, "TWO_D_U", HepEVD::TWO_D_U);
    PyModule_AddIntConstant(hit_type_module, "TWO_D_V", HepEVD::TWO_D_V);
    PyModule_AddIntConstant(hit_type_module, "TWO_D_W", HepEVD::TWO_D_W);

    // Add the submodules to the main module.
    PyModule_AddObject(module, "HIT_DIM", hit_dimension_module);
    PyModule_AddObject(module, "HIT_TYPE", hit_type_module);

    return module;
}
