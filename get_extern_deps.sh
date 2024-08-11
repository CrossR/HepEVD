#!/bin/bash

SUCCESS=0
FAIL=1

GRN='\033[1;32m'
YEL='\033[1;33m'
RED='\033[0;31m'
NC='\033[0m' # No Colour

EXTERN_PATH="include/extern"

HTTPLIB_URL="https://raw.githubusercontent.com/yhirose/cpp-httplib/master/httplib.h"
HTTPLIB_FILE_PATH="${EXTERN_PATH}/httplib.h"

JSON_URL="https://raw.githubusercontent.com/nlohmann/json/develop/single_include/nlohmann/json.hpp"
JSON_PATH="${EXTERN_PATH}/json.hpp"

JS_LIBS=(
    "https://cdn.tailwindcss.com/3.4.5"
    "https://cdn.jsdelivr.net/npm/daisyui@4.12.10/dist/full.min.css"
    "https://cdn.jsdelivr.net/npm/theme-change@2.5.0/index.js"
    "https://ga.jspm.io/npm:es-module-shims@1.10.0/dist/es-module-shims.js"
    "https://cdn.jsdelivr.net/npm/katex@0.16.11/dist/katex.js"
)
JS_LIB_NAMES=(
    "tailwindcss.js"
    "daisyui.css"
    "theme-change.js"
    "es-module-shims.js"
    "katex.js"
)
JS_PATH="web/vendor"

# Three.js is fairly large (about 350 MB).
# So, two options: Dynamicall load it from a CDN, or download it.
# Basically trading a slight delay for a large file size.
THREEJS_VERSION="167"
THREEJS_URL="https://github.com/mrdoob/three.js/archive/refs/tags/r${THREEJS_VERSION}.tar.gz"
DOWNLOAD_THREEJS=false
PATCH_THREEJS=false

# Define the local and CDN paths for the three.js module and addons.
# That way, we can swap between the two easily.
THREEJS_MODULE_LOCAL="./vendor/threejs/build/three.module.min.js"
THREEJS_ADDONS_LOCAL="./vendor/threejs/examples/jsm/"

THREEJS_MODULE_CDN="https://cdn.jsdelivr.net/npm/three@0.${THREEJS_VERSION}.0/build/three.module.min.js"
THREEJS_ADDONS_CDN="https://cdn.jsdelivr.net/npm/three@0.${THREEJS_VERSION}.0/examples/jsm/"

LogMessage() {
    echo -e "${GRN}$(date +'%d/%m/%Y %T') : INFO    : $*${NC}"
}

LogWarning() {
    echo -e "${YEL}$(date +'%d/%m/%Y %T') : WARN    : $*${NC}"
}

LogError() {
    echo -e "${RED}$(date +'%d/%m/%Y %T') : ERR     : $*${NC}"
}

RunAndCheck() {
    eval "$1"
    RETURN_CODE=$?

    if [ $RETURN_CODE -ne 0 ] && [ "$3" = "ERR" ]; then
        LogError "$2"
        exit ${FAIL}
    elif [ $RETURN_CODE -ne 0 ] && [ "$3" = "WARN" ]; then
        LogWarning "$2"
    fi
}

show_help() {

    cat <<EOF
USAGE
    get_extern_deps.sh

    Pull down the required external dependencies, into include/extern and web/vendor.

OPTIONS
    -h
        Show this help message.

    -T
        Download a local copy of Three.js, rather than using a CDN.
        This is a ~350 MB download, but you'll have a local copy of the library,
        making development easier, and saving a bit of time on every page load.

    -r
        Revert back to the CDN version of Three.js, rather than the local copy.
        This doesn't delete the local copy, just changes the HTML to use the CDN
        version. This behaviour is the default, so if you hit any issues you
        can also just use git to revert the changes to web/index.html with:
            git checkout -- web/index.html
EOF
}

while getopts "h?Tr" opt; do
    case $opt in
    h)
        show_help
        exit ${SUCCESS}
        ;;
    \?)
        show_help
        exit ${FAIL}
        ;;
    T)
        DOWNLOAD_THREEJS=true
        ;;
    r)
        PATCH_THREEJS=true
        ;;
    esac
done

if [ "${PATCH_THREEJS}" = true ]; then
    LogMessage "Patching Three.js to use the CDN version..."
    sed -i "s|${THREEJS_MODULE_LOCAL}|${THREEJS_MODULE_CDN}|" web/index.html
    sed -i "s|${THREEJS_ADDONS_LOCAL}|${THREEJS_ADDONS_CDN}|" web/index.html
    LogMessage "Done patching!"

    if [ -d "./web/vendor/three.js" ]; then
        LogMessage "You may want to delete the local copy of Three.js, to save space."
        LogMessage "It lives in ./web/vendor/three.js"
    fi
    exit ${SUCCESS}
fi

LogMessage "Making include/extern folder..."
RunAndCheck "mkdir -p include/extern" \
    "Failed to make extern folder!" \
    "ERR"
LogMessage "Making ${JS_PATH} folder..."
RunAndCheck "mkdir -p ${JS_PATH}" \
    "Failed to make extern folder!" \
    "ERR"

LogMessage "Getting httplib.h..."
RunAndCheck "wget ${HTTPLIB_URL} -O ${HTTPLIB_FILE_PATH} " \
    "Failed to download httplib.h!" \
    "ERR"

LogMessage "Getting json.hpp..."
RunAndCheck "wget ${JSON_URL} -O ${JSON_PATH} " \
    "Failed to download json.hpp!" \
    "ERR"

LogMessage "Getting JS libs..."
for i in "${!JS_LIBS[@]}"; do

    JS_LIB=${JS_LIBS[$i]}
    JS_NAME=${JS_LIB_NAMES[$i]}

    LogMessage "Getting ${JS_LIB}..."
    RunAndCheck "wget ${JS_LIB} -O ${JS_PATH}/${JS_NAME}" \
        "Failed to download ${JS_LIB}!" \
        "WARN"
done

if [ "${DOWNLOAD_THREEJS}" = true ]; then

    LogMessage "Downloading Three.js..."
    RunAndCheck "wget ${THREEJS_URL} -O ${JS_PATH}/threejs.tar.gz" \
        "Failed to download Three.js!" \
        "ERR"

    RunAndCheck "mkdir -p ${JS_PATH}/threejs" \
        "Failed to make ${JS_PATH}/threejs folder!" \
        "ERR"
    RunAndCheck "tar -xzf ${JS_PATH}/threejs.tar.gz -C ${JS_PATH}/threejs --strip-components=1" \
        "Failed to extract ${JS_LIB}!" \
        "ERR"
    RunAndCheck "rm ${JS_PATH}/threejs.tar.gz" \
        "Failed to tidy up ${JS_PATH}/threejs.tar.gz!" \
        "WARN"

    # Now we need to patch the HTML to use the local version of Three.js
    LogMessage "Patching HTML to use local Three.js..."
    sed -i "s|${THREEJS_MODULE_CDN}|${THREEJS_MODULE_LOCAL}|" web/index.html
    sed -i "s|${THREEJS_ADDONS_CDN}|${THREEJS_ADDONS_LOCAL}|" web/index.html
    LogMessage "Done patching!"
fi

LogMessage "Done!"

exit ${SUCCESS}
