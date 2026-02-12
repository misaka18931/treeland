#!/usr/bin/env bash

set -e
DATE=$(date +%m%d_%H%M%S)
SCRIPT_DIR="$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &>/dev/null && pwd )"
cd "$SCRIPT_DIR"

source ./env.sh

# tar -czf /tmp/$PROJECT.tar.gz \
#     --exclude-vcs \
#     --exclude build \
#     --clamp-mtime \
#     -C .. .

# scp /tmp/$PROJECT.tar.gz "root@$REMOTE_HOST":/tmp/

rsync -avz \
    --exclude=.git \
    --exclude=build \
    --exclude=.cmake \
    ../ "root@$REMOTE_HOST":/root/ut-builds/$PROJECT/

# ssh "root@$REMOTE_HOST" bash -c "'
# set -e
# mkdir -p /root/ut-builds/$PROJECT
# tar -xzf /tmp/$PROJECT.tar.gz -C /root/ut-builds/$PROJECT
# rm -f /tmp/$PROJECT.tar.gz
# '"
ssh "root@$REMOTE_HOST" "/root/ut-builds/$PROJECT/scripts/build.sh $@"
ssh "root@$REMOTE_HOST" "systemctl restart treeland"
ssh "root@$REMOTE_HOST"
