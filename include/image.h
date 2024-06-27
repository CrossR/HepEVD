//
// Image
//
// Represent a raw image, given as a 2D array of pixel values.

#ifndef HEP_EVD_IMAGE_H
#define HEP_EVD_IMAGE_H

#include "extern/json.hpp"
using json = nlohmann::json;

namespace HepEVD {

// High level Class representing an image.
// This is a 2D array of pixel values.
class Image {
  public:
    Image() {}
    Image(std::vector<std::vector<float>> &image, const std::string label = "") {

        if (image.size() == 0)
            throw std::invalid_argument("Image must have at least one row!");

        this->data = image;

        this->width = image[0].size();
        this->height = image.size();
        this->label = label;
    }

    NLOHMANN_DEFINE_TYPE_INTRUSIVE(Image, width, height, data, label);

  protected:
    std::vector<std::vector<float>> data;
    int width;
    int height;
    std::string label;
};

using Images = std::vector<Image*>;

inline static void to_json(json &j, const Images &images) {

    if (images.size() == 0) {
        j = json::array();
        return;
    }

    for (const auto &image : images) {
        j.push_back(*image);
    }
}

inline static void from_json(const json &j, Images &images) {
    if (!j.is_array())
        throw std::invalid_argument("Images must be an array!");

    for (const auto &jsonImage : j) {
        images.push_back(new Image(jsonImage));
    }
}

}; // namespace HepEVD

#endif // HEP_EVD_IMAGE_H
