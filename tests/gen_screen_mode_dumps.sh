#!/usr/bin/env bash
set -eu

top_builddir="${abs_top_builddir:-$(pwd)}"
bin="${top_builddir}/g15stats"
workdir="${top_builddir}/.tmp/screen-dumps"
rawdir="${workdir}/raw"
readable_dir="${workdir}/readable"
png_dir="${workdir}/png"
converter_src="${top_builddir}/tests/convert_g15_frames.c"
converter_bin="${workdir}/convert_g15_frames"

screens="${SCREENS:-0 1 2 3 4 5 6 7 8 9 10 11 12}"
modes="${MODES:-0 1}"
timeout_s="${TIMEOUT_SECONDS:-6}"

mkdir -p "$rawdir" "$readable_dir"
mkdir -p "$png_dir"

cc "$converter_src" -o "$converter_bin" -lg15render

for screen in $screens; do
    for mode in $modes; do
        outfile="$rawdir/screen_${screen}_mode_${mode}.bin"
        runlog="$rawdir/screen_${screen}_mode_${mode}.log"
        outdir="$readable_dir/screen_${screen}_mode_${mode}"

        mkdir -p "$outdir"
        rm -f "$outfile" "$runlog"

        set +e
        timeout "${timeout_s}s" env G15STATS_PAUSE=1 G15STATS_FORCE_SCREEN="$screen" G15STATS_FORCE_MODE="$mode" "$bin" -o "$outfile" -r 1 -ir >"$runlog" 2>&1
        rc=$?
        set -e

        if [ "$rc" -ne 124 ]; then
            echo "screen $screen mode $mode: expected timeout exit (124), got: $rc"
            exit 1
        fi

        if [ ! -s "$outfile" ]; then
            echo "screen $screen mode $mode: output file is missing or empty"
            exit 1
        fi

        if grep -q "cant connect to the G15daemon" "$runlog"; then
            echo "screen $screen mode $mode: output-file mode should not require g15daemon"
            exit 1
        fi

        "$converter_bin" "$outfile" "$outdir"

        if [ ! -s "$outdir/frame_000.pbm" ] || [ ! -s "$outdir/frame_000.txt" ]; then
            echo "screen $screen mode $mode: readable frame conversion failed"
            exit 1
        fi

        python3 - "$outdir/frame_000.pbm" "$png_dir/screen_${screen}_mode_${mode}.png" <<'PY'
import sys
from PIL import Image

in_path = sys.argv[1]
out_path = sys.argv[2]

img = Image.open(in_path).convert('1')
img = img.resize((img.width * 3, img.height * 3), Image.NEAREST)
img.save(out_path)
PY

        if [ ! -s "$png_dir/screen_${screen}_mode_${mode}.png" ]; then
            echo "screen $screen mode $mode: png conversion failed"
            exit 1
        fi

        echo "ok: screen ${screen} mode ${mode} -> png"
    done
done

echo "dumps written to: $workdir"
