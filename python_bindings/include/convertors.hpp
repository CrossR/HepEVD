//
// Object Convertors
//

#ifndef CONVERTORS_HPP
#define CONVERTORS_HPP

#include <Python.h>

#include "hep_evd.h"

// Custom converter for parsing out hits.
static int HitConverter(PyObject *hitObj, HepEVD::Hit **result) {

    // Check if we have a tuple, and if it has the right number of elements.
    PyObject *obj = hitObj;

    if (PyArray_Check(hitObj)) {
        obj = PyArray_ToList((PyArrayObject *)hitObj);
    }

    if (!PyList_Check(obj) || PyList_Size(obj) < 4 || PyList_Size(obj) > 6) {
        std::cout << "HepEVD: Failed to validate hit tuple." << std::endl;
        std::cout << "HepEVD: Tuple had " << PyList_Size(obj)  << "entries." << std::endl;
        return 0;
    }

    // Parse out the position and energy.
    double x(PyFloat_AsDouble(PyList_GetItem(obj, 0)));
    double y(PyFloat_AsDouble(PyList_GetItem(obj, 1)));
    double z(PyFloat_AsDouble(PyList_GetItem(obj, 2)));
    double energy(PyFloat_AsDouble(PyList_GetItem(obj, 3)));

    // These are both optional, so we need to check if they exist.
    HepEVD::HitDimension dimension(HepEVD::HitDimension::THREE_D);
    if (PyList_Size(obj) >= 5)
        dimension = static_cast<HepEVD::HitDimension>(PyLong_AsLong(PyList_GetItem(obj, 4)));

    HepEVD::HitType hitType(HepEVD::HitType::GENERAL);
    if (PyList_Size(obj) >= 6)
        hitType = static_cast<HepEVD::HitType>(PyLong_AsLong(PyList_GetItem(obj, 5)));

    // Create a new hit, and store it in the result.
    HepEVD::Hit *hit = new HepEVD::Hit({x, y, z}, energy);
    hit->setDim(dimension);
    hit->setHitType(hitType);

    *result = hit;

    return 1;
}

#endif