# TODO

picoperl: microperl を RP2350 (Cortex-M33) 向け最小 Perl にする作業リスト。
詳細な方針は [CLAUDE.md](CLAUDE.md) を参照。

## 現状 (2026-09-12, x86_64 WSL/Debian)

- `./make-picoperl.sh` でビルド成功、`picoperl -e` 動作確認済み
- バイナリサイズ: 909,832 bytes (`-Os -flto -ffunction-sections -fdata-sections` + `--gc-sections`)
- NV は float 化済み: `nvtype='float'` / `nvsize='4'` / `ivsize='8'`
- `NV_DIG`/`NV_MANT_DIG`/`NV_MIN`/`NV_MAX`/`NV_EPSILON` も `FLT_*` 基準に修正済み
  (Phase 3)。数値の文字列化(`print`/`sprintf`のデフォルト精度)が float 相当になった
- `./picoperl test-float.pl` は `# NV=float` + `ALL TESTS PASSED`(stringify precision
  テストを追加)
- 未定義 libc シンボル: 104 個 (`nm -u picoperl`)。Phase 2/3 で `sinf` 等の float 版
  libm 関数が増えた一方、`floor`/`ceil`/`fmod` は double 版もまだ別箇所から
  直接呼ばれており両方リンクされている(Phase 4 で要精査)

## Phase 1: 足場固め

- [x] `CLAUDE.md` と `test-float.pl` をコミットする
- [x] `.gitignore` に `*.o` と `/picoperl-*/picoperl` を追加
- [x] `test-float.pl` を `make-picoperl.sh` 実行後に自動で走らせる

## Phase 2: NV を float にする

- [x] `make-picoperl.sh` の `uconfig.sh` 生成後に `nvtype='float'` / `nvsize='4'`
      を上書きする1行を追加(`d_longdbl` は元々 `undef` なので変更不要)
- [x] `uconfig.sh`: `nvtype='float'` / `nvsize='4'` / `d_longdbl='undef'`
- [x] `config_h.SH` 経由の `NV_DIG` / `NV_MANT_DIG` / `NV_MAX` / `NV_MIN` /
      `NV_EPSILON` を確認 → **float 相当にならず、常に `DBL_*` にハードコード**
      されている(`perl.h` の `USE_LONG_DOUBLE` 分岐の `#else` 側)。
      test-float.pl は数値の文字列化(sprintf %.*g)を経由しないため今回は
      未着手のまま Phase 3 へ持ち越し。stringify する処理(`sprintf`,
      `Perl_sv_vcatpvfn` 経由の `%f`/`%g` 出力)を追加するテストを書く際は
      要注意
- [x] IV 側を確認 → `ivsize=8` のまま(x86_64 host の `long` が 8byte のため)。
      RP2350 は 32bit なので Phase 6/7 のクロスビルド時に自然と 4 になる想定。
      いまは変更不要と判断
- [x] `./picoperl test-float.pl` が `# NV=float` + `ALL TESTS PASSED` になること

### Phase 2 で見つかった2つの実バグ(修正済み)

1. **`pp.c:2837`**: `NV (*func)(NV) = Perl_sin;` で `Perl_sin` が `#define Perl_sin sin`
   (`double sin(double)`) のままだと、`float(*)(float)` 型の関数ポインタに
   `double(double)` 関数を代入することになり呼び出し規約が壊れる
   (`sqrt(2)` が `0`、`sin(1)` が `1` を返す不正動作)。
   → `perl.h` に `#if NVSIZE == 4` 分岐を追加し、`sinf`/`cosf`/`sqrtf`/`expf`/
   `logf`/`atan2f`/`powf`/`floorf`/`ceilf`/`fmodf`/`modff`/`frexpf` を使うよう修正
   (`make-picoperl.sh` から `perl -0777 -pi -e` で自動パッチ)
2. **`sv.c` の `bodies_by_type[SVt_NV]`**: 空き body の連結に `*(void**)start = next`
   で 8byte のポインタを書き込む (`S_more_bodies`) が、`body_size` が
   `sizeof(NV)`(=4byte)しかないため隣接領域を破壊しセグフォルトしていた。
   → `body_size` を `sizeof(NV)` と `sizeof(char*)` の大きい方にするよう
   `sv.c` を自動パッチ。RP2350 (32bit, ポインタも4byte) では本来この問題は
   起きないが、x86_64 (ポインタ8byte) での中間検証に必須の修正
3. `test-float.pl` の `2^24+1 precision` テストが誤設計だった: `$i+=0.0` は
   Perl の `PERL_PRESERVE_IVUV` 整数最適化により NV 変換を経由せず整数のまま
   計算されるため float/double の差が出ない。`$i=16777217.0` (NV リテラル
   直接代入)に変更して修正

## Phase 3: double 依存を言語側から外す

- [x] libm を float 版へ (`sinf` / `cosf` / `sqrtf` / `powf` / `expf` / `logf` /
      `atan2f` / `fmodf` / `floorf` / `ceilf` / `frexpf` / `modff`)
      → **Phase 2 のバグ修正で対応済み**(`perl.h` の `#if NVSIZE == 4` 分岐)
- [x] `perl.h`: `NV_DIG` / `NV_MANT_DIG` / `NV_MIN` / `NV_MAX` / `NV_MIN_10_EXP` /
      `NV_MAX_10_EXP` / `NV_EPSILON` が Phase 2 完了時点でも `DBL_*` のままだった点を
      修正。`#if NVSIZE == 4` で `FLT_*` を使うよう分岐追加
      (`make-picoperl.sh` で自動パッチ。**Perl_sin 等のパッチより後に実行する必要
      がある** — `#if NVSIZE == 4` の目印をそのパッチの挿入結果に依存しているため)
  - 副次的に発覚: `i_float='undef'` のため `<float.h>` が include されておらず
    `FLT_DIG` 等が未定義でコンパイルエラーになった。`DBL_DIG` だけは `perl.h` 内に
    `#ifndef HAS_DBL_DIG / #define DBL_DIG 15` というフォールバックがあり今まで
    問題が隠れていた。`uconfig.sh` の `i_float` と `d_dbl_dig` を `define` に変更
    (glibc の `<float.h>` はマクロのみでランタイム依存が増えないため副作用なし)
  - 効果を確認: `print 1/3` が `0.333333343267441`(15桁, DBL_DIG基準の無意味な
    ゴミ桁込み)→ `0.333333`(6桁, FLT_DIG基準)に改善。`test-float.pl` に
    `stringify precision (NV_DIG)` テストを追加して固定
- [x] `sv.c`: `sv_2nv` / `Perl_sv_vcatpvfn` の浮動小数変換経路 → 確認した限り
      問題なし。可変長引数(`sprintf`/`printf`)への float 引数は C の既定の実引数
      昇格で自動的に double 化されるため、`vsprintf` 側の処理を変更する必要はない。
      `printf("%.10f", 0.1)` が `0.1000000015` になるのは「float に格納された時点で
      失われた精度を昇格後にそのまま見せている」だけで正しい動作
- [x] `numeric.c`: `Perl_my_atof` / `grok_number` / `Perl_my_atof2` → 確認した限り
      問題なし。`MAX_SIG_DIGITS (NV_DIG+2)` は `NV_DIG` 修正により 17→8 に自動で
      追従した(桁数が多い数値リテラルの丸めを手動テストし回帰なしを確認)
- [x] `pp.c`: `pp_pow` / `pp_sin` / `pp_sqrt` など数値オペコード → `pp_sin`
      (`sin`/`cos`/`exp`/`log`/`sqrt`共通実装)の関数ポインタは Phase 2 で修正済み。
      `pp_pow`・`pp_int`・`pp_rand` は `Perl_pow`/`Perl_floor`/`Perl_ceil` 等の
      マクロ経由で直接呼んでいるだけなので関数ポインタ問題は無く、動作確認のみで
      修正不要だった

## Phase 4: libc 依存の削減 → libc-pico2/ フォルダを作成し *.h *.c を作成

- [ ] `floor`/`ceil`/`fmod`(double版)が `floorf`/`ceilf`/`fmodf` と両方リンク
      されている(`nm -u` で確認)。`Perl_floor`等のマクロ経由以外にも
      `pp_pack.c`/`numeric.c`/`time64.c` あたりで直接 `floor()` 等を呼んでいる
      箇所がある想定。float 版に統一できるか、struct tm 計算など double が
      本質的に必要な箇所かを切り分ける
- [ ] プロセス系を無効: `fork` `execl` `execv` `execvp` `wait` `kill` `pipe`
      `sleep` `getpid` `getuid` `geteuid` `getgid` `getegid` `setuid` `setgid`
      (主に `pp_sys.c` / `doio.c`) → 常にエラーを返すマクロ実装に置き換える
- [ ] ファイル系を RAMFS 前提に: `open` `close` `read` `write` `lseek` `stat`
      `fstat` `opendir` `readdir` `closedir` `chdir` `chmod` `rename` `unlink`
      `umask` `dup` `isatty` `tmpfile`
- [ ] stdio を PerlIO 経由で UART に直結: `fopen` `fclose` `fread` `fwrite`
      `fgetc` `fputs` `fprintf` `fflush` `fseek` `ftell` `feof` `ferror`
      `clearerr` `fileno` `fdopen` `ungetc` `stdin` `stdout` `stderr`
- [ ] 環境変数の実装: `getenv` `putenv` → freeしないハッシュに格納する
- [ ] `qsort` → `pp_sort.c` 内製ソートに寄せる
- [ ] `rand` / `srand` → 内製 PRNG に置き換え
- [ ] `localtime` / `time` → `time64.c` + 固定エポックの時刻を返す
- [ ] `malloc` / `calloc` / `realloc` / `free` → 固定ヒープアロケータ
- [ ] `__ctype_b_loc` (locale 依存) を外す → `locale.c` の除去とセット
- [ ] `setjmp` / `longjmp` は Cortex-M33 でも必要。newlib-nano 版で確認

## Phase 5: ARM Linux/Thumb で中間検証

- [ ] `arm-linux-gnueabihf-gcc -mthumb` でクロスビルド
- [ ] `qemu-arm` 上で `test-float.pl` を通す
- [ ] サイズを記録して x86_64 版と比較

## Phase 6: arm-none-eabi / Cortex-M33 (RP2350)

- [ ] `arm-none-eabi-gcc -mcpu=cortex-m33 -mthumb` + newlib-nano
- [ ] リンカスクリプト / スタートアップ / スタックサイズの決定
- [ ] ヒープサイズと RP2350 の RAM (520KB) に収まるかの見積もり
- [ ] `setjmp`/`longjmp` と例外処理の動作確認

## Phase 7: RAMFS + UART のみで動作

- [ ] スクリプトをバイナリに埋め込む RAMFS を実装
- [ ] PerlIO を UART ドライバに接続 (stdin/stdout/stderr のみ)
- [ ] QEMU libvirt で起動確認

## 検証コマンド

```sh
./make-picoperl.sh
cd picoperl-5.12.5
./picoperl -e 'print 0.123, "OK\n"'
./picoperl ../test-float.pl
nm -u picoperl | wc -l
wc -c < picoperl
```
