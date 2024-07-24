#include <nanobind/nanobind.h>
#include <nanobind/ndarray.h>

namespace nb = nanobind;

/**
 * Check if the given object is an ndarray or a list.
 *
 * @param obj The object to check
 *
 * @return True if the object is an ndarray or a list, false otherwise
 *
 * @throws Any exception that occurs during the type check
 */
bool isArrayOrList(nb::handle obj) { return nb::isinstance<nb::ndarray<>>(obj) || nb::isinstance<nb::list>(obj); }

/**
 * Retrieves items from the given object based on index and size.
 *
 * @param obj The object to extract items from
 * @param index The starting index for extraction
 * @param size The number of items to extract
 *
 * @return A vector containing the extracted items
 *
 * @throws std::runtime_error if the input type is unknown
 */
template <typename T> std::vector<T> getItems(nb::handle obj, int index, int size) {

    if (!isArrayOrList(obj))
        throw std::runtime_error("HepEVD: Object must be an array or list");

    if (nb::isinstance<nb::ndarray<>>(obj)) {
        nb::ndarray<> array = nb::cast<nb::ndarray<>>(obj);

        T *data = static_cast<T *>(array.data());
        std::vector<T> items;

        for (int i = 0; i < size; i++)
            items.push_back(data[index * size + i]);

        return items;

    } else if (nb::isinstance<nb::list>(obj)) {
        nb::list list = nb::cast<nb::list>(obj);

        try {
            if (index < list.size() && isArrayOrList(list[index]))
                return getItems<T>(list[index], index, size);
        } catch (...) {
        }

        std::vector<T> items;

        for (int i = 0; i < size; i++)
            items.push_back(nb::cast<T>(list[i]));

        return items;
    }

    throw std::runtime_error("HepEVD: Unknown object type");
}

using BasicSizeInfo = std::vector<int>;
/**
 * Retrieves the basic size information from the input handle object.
 *
 * @param obj The handle object to extract size information from
 *
 * @return A vector containing the basic size information
 *
 * @throws std::runtime_error if the input type is unknown
 */
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
