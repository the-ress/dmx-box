#!/bin/sh
command -v podman >/dev/null && DOCKER=podman

: "${DOCKER:=docker}"
: "${DMXBOX_DEVICE:=/dev/ttyUSB0}"
: "${DMXBOX_MOUNTPOINT:=/home/ubuntu/project}"

CDPATH= cd -- "$(dirname -- "$0")"/..
exec $DOCKER run \
  --rm \
  --interactive \
  --tty \
  --volume ".:${DMXBOX_MOUNTPOINT}" \
  --device "${DMXBOX_DEVICE}" \
  --userns=keep-id \
  --group-add keep-groups \
  --workdir "${DMXBOX_MOUNTPOINT}" \
  dmxbox
  "$@"
