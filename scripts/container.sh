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
    --env HOME=/tmp \
    --device "${DMXBOX_DEVICE}" \
    --group-add keep-groups \
    "$@"
}

run_ui() {
  run_docker \
    $1 \
    --workdir /project/dmxui \
    node:22-alpine \
    /bin/sh -c "$2"
}

run_idf() {
  run_docker \
    --workdir /project \
    $1 \
    espressif/idf:release-${ESPIDF_RELEASE} \
    /bin/sh -c "$2"
}

build_ui() {
  run_ui '' 'npm install && npm run clean && npm run build:nomap'
}

build_main() {
  run_idf '' 'idf.py build'
}

case "$1" in
  ui)
    build_ui
    ;;
  build) 
    build_ui
    build_main
    ;;
  flash)
    build_ui
    build_main
    run_idf '--tty --interactive' \
      "idf.py flash --port ${DMXBOX_DEVICE} && idf.py monitor --port ${DMXBOX_DEVICE}"
    ;;
  monitor)
    run_idf '--tty --interactive' "idf.py monitor --port ${DMXBOX_DEVICE}"
    ;;
  tty)
    run_idf '--tty --interactive' 'sh'
    ;;
esac
