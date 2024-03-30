//
// Object Convertors
//

#ifndef CONVERTORS_HPP
#define CONVERTORS_HPP

#include <Python.h>

#include "hep_evd.h"

// Custom converter for parsing out hits.
// Templated to allow for different types of hits.
template <typename T> static int HitConverter(PyObject *hitObj, T **result) {

    // Check if we have a tuple, and if it has the right number of elements.
    PyObject *obj = hitObj;

    if (PyArray_Check(hitObj)) {
        obj = PyArray_ToList((PyArrayObject *)hitObj);
    }

    int pos = 0;

    // Tuple element is composed of:
    //    - x, y, z
    //    - pdg? (for MC hits)
    //    - energy?
    //    - dimension?
    //    - hit type?
    int tupleMaxSize(std::is_same<T, HepEVD::MCHit>::value ? 7 : 6);

    if (!PyList_Check(obj) || PyList_Size(obj) < 4 || PyList_Size(obj) > tupleMaxSize) {
        std::cout << "HepEVD: Failed to validate hit tuple." << std::endl;
        std::cout << "HepEVD: Tuple had " << PyList_Size(obj) << " entries." << std::endl;
        return 0;
    }

    // Parse out the position and energy.
    double x(PyFloat_AsDouble(PyList_GetItem(obj, pos++)));
    double y(PyFloat_AsDouble(PyList_GetItem(obj, pos++)));
    double z(PyFloat_AsDouble(PyList_GetItem(obj, pos++)));
    double pdg(0.0);

    if (std::is_same<T, HepEVD::MCHit>::value) {
        pdg = PyFloat_AsDouble(PyList_GetItem(obj, pos++));
    }

    double energy(PyFloat_AsDouble(PyList_GetItem(obj, pos++)));

    // These are both optional, so we need to check if they exist.
    HepEVD::HitDimension dimension(HepEVD::HitDimension::THREE_D);
    if (PyList_Size(obj) >= pos + 2)
        dimension = static_cast<HepEVD::HitDimension>(PyLong_AsLong(PyList_GetItem(obj, pos++)));

    HepEVD::HitType hitType(HepEVD::HitType::GENERAL);
    if (PyList_Size(obj) >= pos + 2)
        hitType = static_cast<HepEVD::HitType>(PyLong_AsLong(PyList_GetItem(obj, pos++)));

    // Create a new hit, and store it in the result.
    T *hit = new T(HepEVD::Position({x, y, z}), energy);

    if constexpr (std::is_same<T, HepEVD::MCHit>::value) {
        hit->setPDG(pdg);
    }

    hit->setDim(dimension);
    hit->setHitType(hitType);

    *result = hit;

    return 1;
}

#endif