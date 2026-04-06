#!/usr/bin/env bash
set -eu

top_builddir="${abs_top_builddir:-$(pwd)}"
bin="${top_builddir}/g15stats"
workdir="${top_builddir}/.tmp/test-output"
outdir="$workdir/screens"
readable_dir="$workdir/readable"
converter_src="${top_builddir}/tests/convert_g15_frames.c"
converter_bin="$workdir/convert_g15_frames"

mkdir -p "$outdir" "$readable_dir"
rm -f "$outdir"/screen_*.bin "$workdir"/run_screen_*.out
rm -rf "$readable_dir"/screen_*

cc "$converter_src" -o "$converter_bin" -lg15render

for screen in 0 1 2 3 4 5 6 7 8; do
    outfile="$outdir/screen_${screen}.bin"
    runlog="$workdir/run_screen_${screen}.out"
    screen_dir="$readable_dir/screen_${screen}"

    mkdir -p "$screen_dir"

    set +e
    timeout 6s env G15STATS_PAUSE=1 G15STATS_FORCE_SCREEN="$screen" "$bin" -o "$outfile" -r 1 -ir >"$runlog" 2>&1
    rc=$?
    set -e

    if [ "$rc" -ne 124 ]; then
        echo "screen $screen: expected timeout exit (124), got: $rc"
        exit 1
    fi

    if [ ! -f "$outfile" ]; then
        echo "screen $screen: output file was not created"
        exit 1
    fi

    size=$(wc -c < "$outfile")
    if [ "$size" -le 0 ]; then
        echo "screen $screen: output file is empty"
        exit 1
    fi

    if grep -q "cant connect to the G15daemon" "$runlog"; then
        echo "screen $screen: file output mode should not require g15daemon"
        exit 1
    fi

    "$converter_bin" "$outfile" "$screen_dir"

    if [ ! -s "$screen_dir/frame_000.pbm" ]; then
        echo "screen $screen: readable PBM frame was not generated"
        exit 1
    fi

    if [ ! -s "$screen_dir/frame_000.txt" ]; then
        echo "screen $screen: readable ASCII frame was not generated"
        exit 1
    fi
done

exit 0
