#!/usr/bin/env bash
set -eu

top_builddir="${abs_top_builddir:-$(pwd)}"
bin="${top_builddir}/g15stats"
workdir="${top_builddir}/.tmp/test-config-variables"
cfg="${workdir}/g15stats.yaml"
cfg_output="${workdir}/from_config.bin"
cli_output="${workdir}/from_cli.bin"
runlog_cfg="${workdir}/run_config.out"
runlog_cli="${workdir}/run_cli.out"

mkdir -p "$workdir"
rm -f "$cfg_output" "$cli_output" "$runlog_cfg" "$runlog_cli"

cat > "$cfg" <<EOF
daemon: false
unicore: true
net_scale_absolute: true
disable_freq: false
info_rotate: true
variable_cpu: true
debug: true
interface: definitely_not_a_real_interface0
output_file: $cfg_output
refresh: 1
temperature: 1
global_temp: 1
fan: 1
EOF

set +e
timeout 6s env G15STATS_CONFIG_FILE="$cfg" G15STATS_FORCE_SCREEN=3 G15STATS_PAUSE=1 "$bin" >"$runlog_cfg" 2>&1
rc_cfg=$?
set -e

if [ "$rc_cfg" -ne 124 ]; then
    echo "expected timeout exit (124) for config-variable run, got: $rc_cfg"
    exit 1
fi

if [ ! -s "$cfg_output" ]; then
    echo "config output_file did not produce a non-empty frame dump"
    exit 1
fi

if grep -q "cant connect to the G15daemon" "$runlog_cfg"; then
    echo "config output_file mode should not require g15daemon"
    exit 1
fi

if ! grep -q "definitely_not_a_real_interface0" "$runlog_cfg"; then
    echo "interface from config was not applied (missing interface-specific logs)"
    exit 1
fi

set +e
timeout 6s env G15STATS_CONFIG_FILE="$cfg" G15STATS_FORCE_SCREEN=3 G15STATS_PAUSE=1 "$bin" -o "$cli_output" >"$runlog_cli" 2>&1
rc_cli=$?
set -e

if [ "$rc_cli" -ne 124 ]; then
    echo "expected timeout exit (124) for cli-override run, got: $rc_cli"
    exit 1
fi

if [ ! -s "$cli_output" ]; then
    echo "CLI -o did not override config output_file"
    exit 1
fi

if grep -q "cant connect to the G15daemon" "$runlog_cli"; then
    echo "CLI output_file mode should not require g15daemon"
    exit 1
fi

exit 0
