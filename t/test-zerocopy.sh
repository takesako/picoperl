#!/bin/sh
# test-zerocopy.sh - romperlのpp_require直結(true zero-copy)経路が、
# 実際にファイルサイズ分のmalloc/reallocを発生させていないことを
# malloctrace.c(LD_PRELOAD)で実測検証する。
#
# 事前に ../romperl で `make` を実行し、romperl本体と
# romperl/rootfs/lib/*.pm (minify.pl生成物) を作っておくこと。
# 通常は `make -C romperl test` の test-zerocopy ターゲットから呼ばれる。
set -eu

SCRIPT_DIR=$(cd "$(dirname "$0")" && pwd)
ROOT_DIR=$(dirname "$SCRIPT_DIR")
ROMPERL_DIR="$ROOT_DIR/romperl"
ROMPERL="$ROMPERL_DIR/romperl"
CC=${CC:-cc}

[ -x "$ROMPERL" ] || {
    echo "error: $ROMPERL が無い。先に \`make -C $ROMPERL_DIR\` を実行すること" >&2
    exit 1
}
[ -d "$ROMPERL_DIR/rootfs/lib" ] || {
    echo "error: $ROMPERL_DIR/rootfs/lib が無い。先に \`make -C $ROMPERL_DIR\` を実行すること" >&2
    exit 1
}

"$CC" -shared -fPIC -o "$SCRIPT_DIR/malloctrace.so" "$SCRIPT_DIR/malloctrace.c" -ldl

LOG=$(mktemp)
trap 'rm -f "$LOG"' EXIT

MALLOCTRACE_MIN=100 LD_PRELOAD="$SCRIPT_DIR/malloctrace.so" "$ROMPERL" \
    -e 'use strict; use warnings; use Carp; print "OK\n";' >/dev/null 2>"$LOG"

# ROMFS上の各モジュール(romperl/rootfs/lib/、minify.pl生成物)のファイル
# サイズに一致するmalloc/reallocが1回も無いことを確認する。サイズは
# ビルドの度にminify.plの出力次第で変わりうるため、固定値ではなく
# rootfs/lib/から都度取得する。
fail=0
for pm in strict Carp Exporter Exporter/Heavy warnings warnings/register; do
    path="$ROMPERL_DIR/rootfs/lib/$pm.pm"
    [ -f "$path" ] || continue
    size=$(wc -c < "$path")
    if grep -Eq "size=${size}([^0-9]|\$)" "$LOG"; then
        echo "NG zero-copy require: $pm.pm (${size} bytes) に一致するmalloc/reallocが発生した" >&2
        fail=1
    else
        echo "ok zero-copy require: $pm.pm (${size} bytes) に一致するmalloc/reallocは発生しない"
    fi
done

if grep -q 'UNDERFLOW' "$LOG"; then
    echo "NG realloc underflow (センチネル不足等でPL_bufendを越えて走査している可能性): " >&2
    grep 'UNDERFLOW' "$LOG" >&2
    fail=1
fi

if [ "$fail" -ne 0 ]; then
    echo "FAILED" >&2
    exit 1
fi
echo "ALL TESTS PASSED"
