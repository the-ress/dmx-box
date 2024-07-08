#!/bin/sh
set -ex
command -v podman >/dev/null && DOCKER=podman

: "${DOCKER:=docker}
: "${IDF_CLONE_BRANCH_OR_TAG:=release/v5.2}
: "${IDF_INSTALL_TARGETS:=esp32}
: "${IDF_CLONE_SHALLOW:=1}

CDPATH= cd -- "$(dirname -- "$0")"
$DOCKER build -t dmxbox/espidf -f Dockerfile.espidf \
  --build-arg "IDF_CLONE_BRANCH_OR_TAG=${IDF_CLONE_BRANCH_OR_TAG}" \
  --build-arg "IDF_CLONE_SHALLOW=1" \
  --build-arg "IDF_INSTALL_TARGETS=esp32"
$DOCKER build -t dmxbox -f Dockerfile

