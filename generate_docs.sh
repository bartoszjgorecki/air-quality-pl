#!/bin/sh
set -eu

cd "$(dirname "$0")"
doxygen Doxyfile
printf 'Doxygen HTML documentation generated in docs/html\n'
