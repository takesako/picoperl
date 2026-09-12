# TODO

picoperl: microperl を RP2350 (Cortex-M33) 向け最小 Perl にする作業リスト。
詳細な方針は [CLAUDE.md](CLAUDE.md) を参照。

## 現状 (2026-09-12, x86_64 WSL/Debian)

- `./make-picoperl.sh` でビルド成功、`picoperl -e` 動作確認済み
- バイナリサイズ: 909,664 bytes (`-Os -flto -ffunction-sections -fdata-sections` + `--gc-sections`)
- NV は still double: `nvtype='double'` / `nvsize='8'` / `ivsize='8'`
- 未定義 libc シンボル: 96 個 (`nm -u picoperl`)

## Phase 1: 足場固め

- [x] `CLAUDE.md` と `test-float.pl` をコミットする
- [x] `.gitignore` に `*.o` と `/picoperl-*/picoperl` を追加
- [x] `test-float.pl` を `make-picoperl.sh` 実行後に自動で走らせる

## Phase 2: NV を float にする

- [ ] `make-picoperl.sh:43` の `perl -MConfig` 置換がホストの `nvtype=double` /
      `nvsize=8` を焼き込んでいる。ここを float 用の上書きに変える
- [ ] `uconfig.sh`: `nvtype='float'` / `nvsize='4'` / `d_longdbl='undef'`
- [ ] `config_h.SH` 経由で決まる `NV_DIG` / `NV_MANT_DIG` / `NV_MAX` / `NV_MIN` /
      `NV_EPSILON` / `NV_INF` / `NV_NAN` が float 相当になるか確認
- [ ] IV 側も見直す (`ivsize=8` のままか、32bit 化するか)
- [ ] `./picoperl test-float.pl` が `# NV=float` + `ALL TESTS PASSED` になること

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
