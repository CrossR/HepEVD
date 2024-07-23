#include <nanobind/nanobind.h>
#include <nanobind/ndarray.h>

namespace nb = nanobind;

bool isArrayOrList(nb::handle obj) { return nb::isinstance<nb::ndarray<>>(obj) || nb::isinstance<nb::list>(obj); }

// Get the number of items in the given list/array.
// In the case of an ndarray, this is simple as they have a data pointer.
// For a list, we need to consider if the list is N-dimensional.
// If it is, we instead can just get the i-th item.
template <typename T> std::vector<T> getItems(nb::handle obj, int index, int size) {
    if (nb::isinstance<nb::ndarray<>>(obj)) {
        nb::ndarray<> array = nb::cast<nb::ndarray<>>(obj);

        T *data = static_cast<T *>(array.data());
        std::vector<T> items;

        for (int i = 0; i < size; i++)
            items.push_back(data[index * size + i]);

        return items;

    } else if (nb::isinstance<nb::list>(obj)) {

        nb::list list = nb::cast<nb::list>(obj);
        nb::handle child = list[index];

        try {
            return getItems<T>(child, index, size);
        } catch (...) {
            std::vector<T> items;

            for (int i = 0; i < size; i++) {
                items.push_back(nb::cast<T>(child[i]));
            }

            return items;
        }
    }

    throw std::runtime_error("HepEVD: Unknown input type!");
}

using BasicSizeInfo = std::vector<int>;
BasicSizeInfo getBasicSizeInfo(nb::handle obj) {

    if (nb::isinstance<nb::ndarray<>>(obj)) {
        nb::ndarray<> array = nb::cast<nb::ndarray<>>(obj);
        return BasicSizeInfo(array.shape_ptr(), array.shape_ptr() + array.ndim());
    } else if (nb::isinstance<nb::list>(obj)) {
        nb::list list = nb::cast<nb::list>(obj);
        BasicSizeInfo size({static_cast<int>(list.size())});
        nb::handle child = list[0];

        if (nb::isinstance<nb::list>(child)) {
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
