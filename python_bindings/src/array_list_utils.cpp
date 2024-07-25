//
// Array / List Utility Functions
//
// Abstract away some of the common array/list operations,
// such that it doesn't matter if the input is an ndarray or
// Python list.

// Local Includes
#include "include/array_list_utils.hpp"

// Include nanobind
#include <nanobind/nanobind.h>
#include <nanobind/ndarray.h>

namespace nb = nanobind;

namespace HepEVD_py {

bool isArrayOrList(nb::handle obj) { return nb::isinstance<nb::ndarray<>>(obj) || nb::isinstance<nb::list>(obj); }

std::vector<double> getItems(nb::handle obj, int index, int size) {

    if (!isArrayOrList(obj))
        throw std::runtime_error("HepEVD: Object must be an array or list");

    if (nb::isinstance<nb::ndarray<>>(obj)) {
        nb::ndarray<> array = nb::cast<nb::ndarray<>>(obj);

        double *data = static_cast<double *>(array.data());
        std::vector<double> items;

        for (int i = 0; i < size; i++)
            items.push_back(data[index * size + i]);

        return items;

    } else if (nb::isinstance<nb::list>(obj)) {
        nb::list list = nb::cast<nb::list>(obj);

        try {
            if (index < list.size() && isArrayOrList(list[index]))
                return getItems(list[index], 0, size);
        } catch (...) {
        }

        std::vector<double> items;

        for (int i = 0; i < size; i++)
            items.push_back(nb::cast<double>(list[i]));

        return items;
    }

    throw std::runtime_error("HepEVD: Unknown object type");
}

BasicSizeInfo getBasicSizeInfo(nb::handle obj) {

    if (nb::isinstance<nb::ndarray<>>(obj)) {
        nb::ndarray<> array = nb::cast<nb::ndarray<>>(obj);
        return BasicSizeInfo(array.shape_ptr(), array.shape_ptr() + array.ndim());
    } else if (nb::isinstance<nb::list>(obj)) {
        nb::list list = nb::cast<nb::list>(obj);
        BasicSizeInfo size({static_cast<int>(list.size())});
        nb::handle child = list[0];

        if (isArrayOrList(child)) {
            try {
                BasicSizeInfo childSize = getBasicSizeInfo(child);
                size.insert(size.end(), childSize.begin(), childSize.end());
            } catch (...) {
            }
        }

        return size;
    }

    throw std::runtime_error("HepEVD: Unknown input type!");
}

} // namespace HepEVD_py
