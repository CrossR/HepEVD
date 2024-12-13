//
// MonochromeImage
//
// Represent a raw image, given as a 2D array of pixel values.

#ifndef HEP_EVD_IMAGE_H
#define HEP_EVD_IMAGE_H

#include "extern/json.hpp"
using json = nlohmann::json;

namespace HepEVD {

enum ImageType { MONOCHROME, RGB };
NLOHMANN_JSON_SERIALIZE_ENUM(ImageType, {{MONOCHROME, "Monochrome"}, {RGB, "RGB"}});

// High level Class representing an image.
// This is a 2D array of pixel values.
class MonochromeImage {
  public:
    MonochromeImage() {}
    MonochromeImage(std::vector<std::vector<float>> &image, const std::string label = "") {

        if (image.size() == 0)
            throw std::invalid_argument("MonochromeImage must have at least one row!");

        this->m_data = image;

        this->m_width = image[0].size();
        this->m_height = image.size();
        this->m_label = label;
    }

    // Define to_json and from_json for MonochromeImage.
    friend void to_json(json &j, const MonochromeImage &image) {
        j["imageType"] = image.m_imageType;
        j["data"] = image.m_data;
        j["width"] = image.m_width;
        j["height"] = image.m_height;
        j["label"] = image.m_label;
    }

    friend void from_json(const json &j, MonochromeImage &image) {
        j.at("imageType").get_to(image.m_imageType);
        j.at("data").get_to(image.m_data);
        j.at("width").get_to(image.m_width);
        j.at("height").get_to(image.m_height);
        j.at("label").get_to(image.m_label);
    }

  protected:
    ImageType m_imageType = MONOCHROME;
    std::vector<std::vector<float>> m_data;
    int m_width;
    int m_height;
    std::string m_label;
};

// TODO: Extend later to include other image types
using Images = std::vector<MonochromeImage *>;

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
        images.push_back(new MonochromeImage(jsonImage));
    }
}

}; // namespace HepEVD

#endif // HEP_EVD_IMAGE_H
