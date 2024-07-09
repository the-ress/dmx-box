#!/bin/sh
set -x
command -v podman >/dev/null && DOCKER=podman

: "${DOCKER:=docker}"
: "${CONTAINER:=dmxbox}"
: "${DEVICE:=/dev/ttyUSB0}"
: "${MOUNTPOINT:=/home/ubuntu/project}"

CDPATH= cd -- "$(dirname -- "$0")"/..

if ! $DOCKER container exists $CONTAINER; then
  $DOCKER container run \
      --detach \
      --device "${DEVICE}" \
      --group-add keep-groups \
      --name "${CONTAINER}" \
      --publish 8000:8000 \
      --rm \
      --userns keep-id \
      --volume ".:${MOUNTPOINT}" \
      dmxbox \
      /bin/tail -f /dev/null \
  || exit 1
fi

$DOCKER container exec \
  --interactive \
  --tty \
  --workdir "${MOUNTPOINT}" \
  $CONTAINER \
  /bin/zsh -c '. /opt/esp/idf/export.sh && exec /bin/zsh'
