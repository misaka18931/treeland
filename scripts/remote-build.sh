#!/usr/bin/env bash

set -e
DATE=$(date +%m%d_%H%M%S)
SCRIPT_DIR="$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &>/dev/null && pwd )"
cd "$SCRIPT_DIR"

source ./env.sh

ssh "root@$REMOTE_HOST" "/root/remote/$PROJECT/scripts/build.sh $@"
ssh "root@$REMOTE_HOST" "systemctl restart treeland"
ssh "root@$REMOTE_HOST"
