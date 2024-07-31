// array_list_utils.hpp

#ifndef HEP_EVD_PY_ARRAY_UTILS_HPP
#define HEP_EVD_PY_ARRAY_UTILS_HPP

// Standard includes
#include <vector>

// Include nanobind
#include <nanobind/nanobind.h>
namespace nb = nanobind;

namespace HepEVD_py {

// The basic shape information of a ndarray / list.
using BasicSizeInfo = std::vector<int>;

/**
 * Check if the given object is an ndarray or a list.
 *
 * @param obj The object to check
 *
 * @return True if the object is an ndarray or a list, false otherwise
 */
bool isArrayOrList(nb::handle obj);

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
std::vector<double> getItems(nb::handle obj, int index, int size);

/**
 * Retrieves the basic size information from the input handle object.
 *
 * @param obj The handle object to extract size information from
 *
 * @return A vector containing the basic size information
 *
 * @throws std::runtime_error if the input type is unknown
 */
BasicSizeInfo getBasicSizeInfo(nb::handle obj);

} // namespace HepEVD_py

#endif // HEP_EVD_PY_ARRAY_UTILS_HPP