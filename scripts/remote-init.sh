#!/usr/bin/env nix
#!nix shell nixpkgs#mutagen --command bash

set -e
DATE=$(date +%m%d_%H%M%S)
SCRIPT_DIR="$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &>/dev/null && pwd )"
cd "$SCRIPT_DIR"

source ./env.sh

ssh $REMOTE_HOST "mkdir -p /root/remote/.build/$PROJECT"
ssh $REMOTE_HOST "mkdir -p /root/remote/$PROJECT"

ssh $REMOTE_HOST "grep -q ${PROJECT}-src /etc/fstab || echo -e '${PROJECT}-overlay\t/root/remote/${PROJECT}\toverlay\tlowerdir=/root/remote/.origin/${PROJECT},upperdir=/root/remote/.local/${PROJECT},workdir=/root/remote/.cache/${PROJECT}\t0\t0' >> /etc/fstab && systemctl daemon-reload && mount -a"

mutagen sync create --mode=one-way-safe --ignore-vcs .. $REMOTE_HOST:/root/remote/.origin/$PROJECT --name sync-${PROJECT}
