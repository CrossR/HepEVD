//
// Configuration
//

#ifndef HEP_EVD_CONFIGURATION_H
#define HEP_EVD_CONFIGURATION_H

#include <cstdlib>
#include <string>

// The current version of HepEVD
#define HEP_EVD_VERSION "1.0.0"

// What port to host the Web UI on?
#ifndef HEP_EVD_PORT
#define HEP_EVD_PORT 5555
#endif

inline int EVD_PORT() {
    if (std::getenv("HEP_EVD_PORT"))
        return std::atoi(std::getenv("HEP_EVD_PORT"));
    return HEP_EVD_PORT;
}

// If the HEP_EVD_WEB_FOLDER env variable is set, use that as the web folder
// Otherwise, build the path to the web folder based on the location of this file
inline std::string WEB_FOLDER() {

    if (std::getenv("HEP_EVD_WEB_FOLDER"))
        return std::getenv("HEP_EVD_WEB_FOLDER");

    const std::string headerFilePath(__FILE__);
    const std::string includeFolder(headerFilePath.substr(0, headerFilePath.rfind("/")));
    const std::string wwwFolder = includeFolder + "/../web/";

    return wwwFolder;
}

#endif // HEP_EVD_CONFIGURATION_H
