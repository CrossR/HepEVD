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

// Standard Includes
#include <iostream>

namespace nb = nanobind;

namespace HepEVD_py {

bool isTensor(nb::handle obj) {
    try {
        nb::object torch = nb::module_::import("torch");
        nb::object tensor_type = torch.attr("Tensor");
        return nb::isinstance(obj, tensor_type);
    } catch (const nb::error_already_set &) {
        return false; // If torch is not available, we cannot check for tensors.
    }
}
bool isArrayOrList(nb::handle obj) {

    if (isTensor(obj)) {
        std::cout << "HepEVD WARN: You've passed a torch tensor, which is not supported by HepEVD. "
                     "Please convert it to a numpy array or a list before passing it to HepEVD.\n"
                  << std::endl;
        return false;
    }

    return nb::isinstance<nb::ndarray<>>(obj) || nb::isinstance<nb::list>(obj);
}

double dtypeToDouble(const void *elem_ptr, char kind, size_t itemsize) {
    switch (kind) {
    case 'f': // float
        if (itemsize == sizeof(float))
            return static_cast<double>(*reinterpret_cast<const float *>(elem_ptr));
        else if (itemsize == sizeof(double))
            return *reinterpret_cast<const double *>(elem_ptr);
        break;
    case 'i': // signed int
        if (itemsize == sizeof(int32_t))
            return static_cast<double>(*reinterpret_cast<const int32_t *>(elem_ptr));
        else if (itemsize == sizeof(int64_t))
            return static_cast<double>(*reinterpret_cast<const int64_t *>(elem_ptr));
        break;
    case 'u': // unsigned int
        if (itemsize == sizeof(uint32_t))
            return static_cast<double>(*reinterpret_cast<const uint32_t *>(elem_ptr));
        else if (itemsize == sizeof(uint64_t))
            return static_cast<double>(*reinterpret_cast<const uint64_t *>(elem_ptr));
        break;
    default:
        throw std::runtime_error("HepEVD: Unsupported dtype for conversion to double");
    }
    throw std::runtime_error("HepEVD: Unhandled dtype/itemsize in convertToFloat");
}

std::vector<double> getItems(nb::handle obj, int index, int size) {

    if (!isArrayOrList(obj))
        throw std::runtime_error("HepEVD: Object must be an array or list");

    if (nb::isinstance<nb::ndarray<>>(obj)) {
        nb::ndarray<> array = nb::cast<nb::ndarray<>>(obj);

        // Work out the array dtypes, so we can convert to double.
        const size_t itemsize = array.dtype().itemsize();
        const char kind = array.dtype().kind();

        const char *data = static_cast<const char *>(array.data());
        std::vector<double> items;

        for (int i = 0; i < size; i++) {
            const void *elem_ptr = data + (index * size + i) * itemsize;
            items.push_back(dtypeToDouble(elem_ptr, kind, itemsize));
        }

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
