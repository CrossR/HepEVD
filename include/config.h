//
// Configuration
//

#ifndef HEP_EVD_CONFIGURATION_H
#define HEP_EVD_CONFIGURATION_H

// The current version of HEPEvd
#define HEP_EVD_VERSION "0.0.1"

// What port to host the Web UI on?
#ifndef HEP_EVD_PORT
#define HEP_EVD_PORT 5555
#endif

// Should we load the Pandora helpers?
#ifndef HEP_EVD_PANDORA_HELPERS
#define HEP_EVD_PANDORA_HELPERS 0
#endif

#endif // HEP_EVD_CONFIGURATION_H
