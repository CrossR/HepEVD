//
// Configuration
//

#ifndef HEP_EVD_CONFIGURATION_H
#define HEP_EVD_CONFIGURATION_H

#include <cstdlib>

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

#endif // HEP_EVD_CONFIGURATION_H
