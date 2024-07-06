#!/bin/sh
command -v podman >/dev/null && DOCKER=podman

: "${DOCKER:=docker}
: "${ESPIDF_RELEASE:=v5.2}
: "${DMXBOX_DEVICE:=/dev/ttyUSB0}"

run_docker() {
  $DOCKER run \
    --rm \
    --tty \
    --volume .:/project \
    --workdir /project \
    --env HOME=/tmp \
    --device "${DMXBOX_DEVICE}" \
    --group-add keep-groups \
    "$@"
}

run_idf() {
  local docker_args=$1
  shift
  run_docker \
    $docker_args \
    "espressif/idf:release-${ESPIDF_RELEASE}" \
    idf.py \
    "$@"
}

case "$1" in
  build) run_idf '' build;;
  flash) run_idf '' flash --port "${DMXBOX_DEVICE}";;
  monitor) run_idf '--tty --interactive' monitor --port "${DMXBOX_DEVICE}";;
esac
