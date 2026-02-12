#!/usr/bin/env bash

set -e
DATE=$(date +%m%d_%H%M%S)
SCRIPT_DIR="$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &>/dev/null && pwd )"
cd "$SCRIPT_DIR"

export PROJECT=$(basename $(realpath ..))
# export REMOTE_HOST="192.168.122.76"
# export REMOTE_HOST="10.20.9.76"
export REMOTE_HOST="deepin-vm"
export BUILD_TYPE="Debug"
export EXTRA_CONFIGURE_ARGS="--preset deb -DCMAKE_CXX_FLAGS=\"-DTREELANDCONFIG_DCONFIG_FILE_VERSION_MINOR=1\""
