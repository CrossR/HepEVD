#!/bin/bash

INPUT_DIRS_CPP=(
    "include"
    "example"
    "python_bindings/src"
)
INPUT_DIRS_JS=("web")

EXCLUDE_DIRS="extern|third_party"

for dir in "${INPUT_DIRS_CPP[@]}"; do
    echo "Formatting C++ files in ${dir}..."
    find "${dir}" -iname "*.c*" -o -iname "*.h*" | grep -vE "${EXCLUDE_DIRS}" | xargs clang-format -i
done

for dir in "${INPUT_DIRS_JS[@]}"; do
    echo "Formatting JS files in ${dir}..."
    prettier --write ${dir}/*.{html,js,css}
done
