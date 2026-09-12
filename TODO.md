# TODO

picoperl: microperl を RP2350 (Cortex-M33) 向け最小 Perl にする作業リスト。
詳細な方針は [CLAUDE.md](CLAUDE.md) を参照。

## 現状 (2026-09-12, x86_64 WSL/Debian)

- `./make-picoperl.sh` でビルド成功、`picoperl -e` 動作確認済み
- バイナリサイズ: 909,832 bytes (`-Os -flto -ffunction-sections -fdata-sections` + `--gc-sections`)
- NV は float 化済み: `nvtype='float'` / `nvsize='4'` / `ivsize='8'`
- `./picoperl test-float.pl` は `# NV=float` + `ALL TESTS PASSED`
- 未定義 libc シンボル: 96 個 (`nm -u picoperl`, Phase 2 時点では未計測見直し)

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

- [ ] `sv.c`: `sv_2nv` / `Perl_sv_vcatpvfn` の浮動小数変換経路
- [ ] `numeric.c`: `Perl_my_atof` / `grok_number` / `Perl_my_atof2`
- [ ] `pp.c`: `pp_pow` / `pp_sin` / `pp_sqrt` など数値オペコード
- [ ] libm を float 版へ (`sinf` / `cosf` / `sqrtf` / `powf` / `expf` / `logf` /
      `atan2f` / `fmodf` / `floorf` / `ceilf` / `frexpf` / `modff`)
- [ ] `sprintf` / `vsprintf` 経由の出力は可変長引数で double に昇格するため、
      この経路自体を自前実装に置き換える必要がある

## Phase 4: libc 依存の削減 (現状 96 シンボル)

- [ ] プロセス系を削る: `fork` `execl` `execv` `execvp` `wait` `kill` `pipe`
      `sleep` `getpid` `getuid` `geteuid` `getgid` `getegid` `setuid` `setgid`
      (主に `pp_sys.c` / `doio.c`)
- [ ] ファイル系を RAMFS 前提に: `open` `close` `read` `write` `lseek` `stat`
      `fstat` `opendir` `readdir` `closedir` `chdir` `chmod` `rename` `unlink`
      `umask` `dup` `isatty` `tmpfile`
- [ ] stdio を PerlIO 経由で UART に直結: `fopen` `fclose` `fread` `fwrite`
      `fgetc` `fputs` `fprintf` `fflush` `fseek` `ftell` `feof` `ferror`
      `clearerr` `fileno` `fdopen` `ungetc` `stdin` `stdout` `stderr`
- [ ] 環境変数を削る: `getenv` `putenv`
- [ ] `qsort` → `pp_sort.c` 内製ソートに寄せる
- [ ] `rand` / `srand` → 内製 PRNG に置き換え
- [ ] `localtime` / `time` → `time64.c` + 固定エポックまたは UART 経由の時刻
- [ ] `malloc` / `calloc` / `realloc` / `free` → 固定ヒープアロケータ
- [ ] `__ctype_b_loc` (locale 依存) を外す → `locale.c` の除去とセット
- [ ] `setjmp` / `longjmp` は Cortex-M33 でも必要。newlib-nano 版で確認

## Phase 5: 不要ソースを FILES から削る

- [ ] `mathoms.c` (`-DNO_MATHOMS` で実質空。FILES から外せるか確認)
- [ ] `locale.c` (Phase 4 の `__ctype_b_loc` 除去とセット)
- [ ] `doio.c` / `pp_sys.c` の縮小 (OS 機能の大半がここ)
- [ ] `taint.c` / `deb.c` / `dump.c` が削れるか確認
- [ ] 削るたびに `./make-picoperl.sh` + `test-float.pl` で回帰確認

## Phase 6: ARM Linux/Thumb で中間検証

- [ ] `arm-linux-gnueabihf-gcc -mthumb` でクロスビルド
- [ ] `qemu-arm` 上で `test-float.pl` を通す
- [ ] サイズを記録して x86_64 版と比較

## Phase 7: arm-none-eabi / Cortex-M33 (RP2350)

- [ ] `arm-none-eabi-gcc -mcpu=cortex-m33 -mthumb` + newlib-nano
- [ ] リンカスクリプト / スタートアップ / スタックサイズの決定
- [ ] ヒープサイズと RP2350 の RAM (520KB) に収まるかの見積もり
- [ ] `setjmp`/`longjmp` と例外処理の動作確認

## Phase 8: RAMFS + UART のみで動作

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
