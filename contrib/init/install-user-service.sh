#!/bin/sh
# Ad-hoc installer for the g15stats systemd user service, for running
# straight out of a source checkout without building the .deb package.

set -e

SCRIPT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
SRC="${SCRIPT_DIR}/g15stats.service"
DEST_DIR="${HOME}/.config/systemd/user"
DEST="${DEST_DIR}/g15stats.service"

mkdir -p "${DEST_DIR}"
ln -sf "${SRC}" "${DEST}"

systemctl --user daemon-reload
systemctl --user enable --now g15stats.service
