#!/usr/bin/env bash

set -e
DATE=$(date +%m%d_%H%M%S)
SCRIPT_DIR="$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &>/dev/null && pwd )"

if [ -f './scripts/env.sh' ]; then
    cp ./scripts/env.sh ./scripts/env.sh.bak
fi
cp -r "$SCRIPT_DIR/" ./

if [ -f './scripts/env.sh.bak' ]; then
    mv ./scripts/env.sh.bak ./scripts/env.sh
fi
