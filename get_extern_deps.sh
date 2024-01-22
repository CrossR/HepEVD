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
    "https://cdn.tailwindcss.com/3.4.1"
    "https://cdn.jsdelivr.net/npm/daisyui@4.6.0/dist/full.css"
    "https://cdn.jsdelivr.net/npm/theme-change@2.5.0/index.js"
    "https://ga.jspm.io/npm:es-module-shims@1.8.0/dist/es-module-shims.js"
    "https://github.com/mrdoob/three.js/archive/refs/tags/r154.tar.gz"
    
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

show_help () {

    cat << EOF
USAGE
    get_extern_deps.sh

    Pull down the required external dependencies, into include/extern.
EOF
}

while getopts "h?" opt;
do
    case $opt in
        h)
            show_help
            return ${SUCCESS} 2> /dev/null || exit ${SUCCESS}
            ;;
        \?)
            show_help
            return ${FAIL} 2> /dev/null || exit ${FAIL}
            ;;
    esac
done

LogMessage "Making include/extern folder..."
RunAndCheck "mkdir -p include/extern" \
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
for JS_LIB in "${JS_LIBS[@]}"; do
    RunAndCheck "wget ${JS_LIB} -P ${JS_PATH}" \
                "Failed to download ${JS_LIB}!" \
                "WARN"
    
    if [[ ${JS_LIB} == *.tar.gz ]]; then
        RunAndCheck "tar -xzf ${JS_PATH}/$(basename ${JS_LIB}) -C ${JS_PATH}" \
                    "Failed to extract ${JS_LIB}!" \
                    "ERR"
        RunAndCheck "rm ${JS_PATH}/$(basename ${JS_LIB})" \
                    "Failed to remove ${JS_LIB}!" \
                    "ERR"
    fi
done

LogMessage "Done!"
return ${SUCCESS} 2> /dev/null || exit ${SUCCESS}

