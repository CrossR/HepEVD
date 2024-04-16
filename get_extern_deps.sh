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
    "https://cdn.tailwindcss.com/3.4.3"
    "https://cdn.jsdelivr.net/npm/daisyui@4.10.2/dist/full.css"
    "https://cdn.jsdelivr.net/npm/theme-change@2.5.0/index.js"
    "https://ga.jspm.io/npm:es-module-shims@1.9.0/dist/es-module-shims.js"
    "https://github.com/mrdoob/three.js/archive/refs/tags/r163.tar.gz"
    "https://cdn.jsdelivr.net/npm/katex@0.16.10/dist/katex.js"
)
JS_LIB_NAMES=(
    "tailwindcss.js"
    "daisyui.css"
    "theme-change.js"
    "es-module-shims.js"
    "threejs"
    "katex.js"
)
JS_PATH="web/vendor"

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

    Pull down the required external dependencies, into include/extern.
EOF
}

while getopts "h?" opt; do
    case $opt in
    h)
        show_help
        return ${SUCCESS} 2>/dev/null || exit ${SUCCESS}
        ;;
    \?)
        show_help
        return ${FAIL} 2>/dev/null || exit ${FAIL}
        ;;
    esac
done

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

    if [[ ${JS_LIB} == *.tar.gz ]]; then
        JS_NAME="${JS_NAME}.tar.gz"
    fi

    LogMessage "Getting ${JS_LIB}..."
    RunAndCheck "wget ${JS_LIB} -O ${JS_PATH}/${JS_NAME}" \
        "Failed to download ${JS_LIB}!" \
        "WARN"

    if [[ ${JS_LIB} == *.tar.gz ]]; then
        JS_NAME=${JS_LIB_NAMES[$i]}
    fi

    if [[ ${JS_LIB} == *.tar.gz ]]; then
        # Extract the tar file to the JS_PATH with the JS_NAME
        RunAndCheck "mkdir -p ${JS_PATH}/${JS_NAME}" \
            "Failed to make ${JS_PATH}/${JS_NAME} folder!" \
            "ERR"
        RunAndCheck "tar -xzf ${JS_PATH}/${JS_NAME}.tar.gz -C ${JS_PATH}/${JS_NAME} --strip-components=1" \
            "Failed to extract ${JS_LIB}!" \
            "ERR"
        RunAndCheck "rm ${JS_PATH}/${JS_NAME}.tar.gz" \
            "Failed to remove ${JS_LIB}!" \
            "ERR"
    fi
done

LogMessage "Done!"

exit ${SUCCESS}
