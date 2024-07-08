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
      --device "${DEVICE}" \
      --detach \
      --group-add keep-groups \
      --name "${CONTAINER}" \
      --userns keep-id \
      --volume ".:${MOUNTPOINT}" \
      --rm \
      dmxbox \
      /bin/tail -f /dev/null \
  || exit 1
fi

CMD="${1:-/bin/zsh}"
shift

$DOCKER container exec \
  --interactive \
  --tty \
  --workdir "${MOUNTPOINT}" \
  $CONTAINER \
  $CMD \
  "$@"


