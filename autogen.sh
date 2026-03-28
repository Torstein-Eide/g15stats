#!/usr/bin/env bash
set -e

mkdir -p "$PWD/.tmp"
export TMPDIR="$PWD/.tmp"

autoreconf -fiv
./configure
make
