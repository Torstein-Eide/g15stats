#!/usr/bin/env bash
set -eu

top_builddir="${abs_top_builddir:-$(pwd)}"
bin="${top_builddir}/g15stats"
workdir="${top_builddir}/.tmp/test-yaml"

mkdir -p "$workdir"

valid_cfg="$workdir/valid.yaml"
invalid_cfg="$workdir/invalid.yaml"

cat > "$valid_cfg" <<'EOF'
daemon: false
interface: eth0
refresh: 15
EOF

cat > "$invalid_cfg" <<'EOF'
daemon: true
interface: [
EOF

valid_output="$workdir/valid.out"
invalid_output="$workdir/invalid.out"

set +e
G15STATS_CONFIG_FILE="$valid_cfg" "$bin" -h >"$valid_output" 2>&1
valid_rc=$?
G15STATS_CONFIG_FILE="$invalid_cfg" "$bin" -h >"$invalid_output" 2>&1
invalid_rc=$?
set -e

if [ "$valid_rc" -ne 0 ]; then
    echo "expected help mode to exit successfully with valid config"
    exit 1
fi

if [ "$invalid_rc" -ne 0 ]; then
    echo "expected help mode to exit successfully with invalid config"
    exit 1
fi

if grep -q "Warning: invalid YAML" "$valid_output"; then
    echo "valid YAML unexpectedly reported as invalid"
    exit 1
fi

if ! grep -q "Config file: $valid_cfg" "$valid_output"; then
    echo "help output did not show expected config file path"
    exit 1
fi

if ! grep -q "Warning: invalid YAML" "$invalid_output"; then
    echo "invalid YAML did not produce parser warning"
    exit 1
fi

exit 0
