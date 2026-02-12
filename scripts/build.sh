#!/usr/bin/env bash

set -e
DATE=$(date +%m%d_%H%M%S)
SCRIPT_DIR="$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &>/dev/null && pwd )"
cd "$SCRIPT_DIR"

source ./env.sh

option_configure=false
for arg in "$@"; do
    if [ "$arg" = "--configure" ]; then
        option_configure=true
        break
    fi
done

[ "$(id -u)" -eq 0 ] || { echo "Please run as root"; exit 1; }

apt-get build-dep -y ..

mkdir -p ../../.build/$PROJECT || true

cd ../../.build/$PROJECT

if [ -f "install_manifest.txt" ]; then
    echo "Cleaning previous installation..."
    xargs rm -f < install_manifest.txt || true
fi

if $option_configure; then
cmake -DCMAKE_INSTALL_PREFIX=/usr \
      "-GNinja" \
      -DCMAKE_INSTALL_SYSCONFDIR=/etc \
      -DCMAKE_INSTALL_LOCALSTATEDIR=/var \
      -DCMAKE_INSTALL_RUNSTATEDIR=/run \
      -DCMAKE_INSTALL_LIBDIR=lib/x86_64-linux-gnu \
      -DCMAKE_VERBOSE_MAKEFILE=ON \
      -DCMAKE_BUILD_TYPE=$BUILD_TYPE \
      -DCMAKE_CXX_FLAGS="${CMAKE_CXX_FLAGS} -Wall -Wextra -Werror -Wno-stringop-overflow" \
      $EXTRA_CONFIGURE_ARGS \
      ../../$PROJECT
fi
cmake --build . --target install
