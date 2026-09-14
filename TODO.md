# TODO

picoperl: microperl を RP2350 (Cortex-M33) 向け最小 Perl にする作業リスト。
詳細な方針は [CLAUDE.md](CLAUDE.md) を参照。

## 現状 (2026-09-13, x86_64 WSL/Debian)

- `./make-picoperl.sh` でビルド成功、`picoperl -e` 動作確認済み
- バイナリサイズ: 908,992 bytes (`-Os -flto -ffunction-sections -fdata-sections` + `--gc-sections`)
- NV は float 化済み: `nvtype='float'` / `nvsize='4'` / `ivsize='8'`
- `NV_DIG`/`NV_MANT_DIG`/`NV_MIN`/`NV_MAX`/`NV_EPSILON` も `FLT_*` 基準に修正済み
  (Phase 3)。数値の文字列化(`print`/`sprintf`のデフォルト精度)が float 相当になった
- `./picoperl ../t/test-float.pl` は `# NV=float` + `ALL TESTS PASSED`(stringify
  precision テストを追加)。テスト一式は `t/` フォルダに集約済み(`test-float.pl`/
  `test-noproc.pl`/`test-inc-require.pl`/`romfs_test.c`/`malloctrace.c`/
  `test-zerocopy.sh`)
- `libc-pico2/` フォルダ(Phase 5)で `fork`/`exec*`/`pipe`/`kill`/`wait*`/`sleep`/
  `get{u,g,eu,eg}id`/`set{u,g}id` を無効化・固定値化。`./picoperl ../t/test-noproc.pl`
  も `ALL TESTS PASSED`
- 未定義 libc シンボル: 88 個 (`nm -u picoperl`, Phase 1時点は96個)。`floor`/`ceil`/
  `fmod` の double 版は `time64.c`(Unixエポック秒→暦分解、Perlの NV とは無関係)
  でのみ直接呼ばれており、doubleが本質的に必要と判明・調査済みでクローズ
- `ramfs/`(書き込み可能インメモリファイルシステム、fopen系API)と`vfs/`
  (romfs優先・romfsフォールバックのディスパッチ層)を追加。`stat`/`unlink`
  と、fopen系一式(`fopen`/`fclose`/`fread`/`fwrite`/`fseek`/`ftell`/
  `feof`/`ferror`/`clearerr`/`fflush`/`fgetc`/`fputs`/`fileno`/`fprintf`)
  をvfsにリダイレクト済み(romperlで動作確認、plain picoperlは無変更)。
  このビルド(`useperlio=undef`)ではPerlの通常の`open()`が`fopen()`に
  直結しており(`sysopen`だけが例外)、fopen系だけで`open`/`print`/
  `<$fh>`/`close`が一通り動く。`open`/`close`/`read`/`write`/`lseek`
  (POSIXの生fd系)は対象外(ユーザー指示: 自作libcに置き換えた際に
  呼ばれないなら実装不要)
  (詳細はPhase 5準備/Phase 5セクション参照)
- `romperl/`(Phase 4)完成: `../picoperl-5.12.5`の`.o`を参照するだけで
  picoperl-5.12.5自体には一切手を入れずに、自作ROMFS形式の埋め込み
  (`.romfs`セクション)、`open/read/seek/close/stat`最小API、
  `@INC`フック経由の`require/use`接続まで実装。`make -C romperl test`で
  一括検証できる(`romfs_test`のAPI単体テスト13項目 +
  `test-inc-require.pl`のPerl統合4項目 + `test-float.pl`/`test-noproc.pl`の
  回帰確認)
- `pp_require`を直接ROMFSに繋ぐtrue zero-copy化も完了(この時だけ
  `picoperl-5.12.5/pp_ctl.c`をmake-picoperl.sh経由でパッチしている)。
  `use strict; use warnings; use Carp;`実行時、ファイルサイズ分の
  `malloc`が実測で1回も発生しないことを確認済み(旧`@INC`フック方式は
  1回発生していた)。実装中に「Perlの文字列APIはSvCUR位置が読める
  `'\0'`であることを前提にしている」という別の暗黙の前提を壊して
  `Out of memory`クラッシュを起こす重大バグに遭遇し、修正済み
  (詳細はPhase 4追加セクション参照)

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

## Phase 4: romperl の作成

* [x] `../picoperl-5.12.5` を参照する形で `romperl` のフォルダに雛形を作成する
      → `romperl/Makefile` が `../picoperl-5.12.5/*.o`(`uperlmain.o`除く)を
      コピーせずそのままリンクする方式にした。ヘッダも `-I../picoperl-5.12.5`
      で直接参照し、コピーは一切しない(ソースツリーの二重化を避けるため)。
      前提として先に `../make-picoperl.sh` を実行して `.o` を生成しておく必要が
      あり、`.o` が無い場合はエラーメッセージを出す
      (最初 `romperl-5.12.5/` に picoperl 一式をコピーする案で作ったが、
      ほぼ全ファイルが重複するため却下し、この参照方式に変更した)
* [x] `miniperlmain.c` を参考に非常に簡素にした `main.c` を作成する
      → `romperl/main.c`。PERL_GLOBAL_STRUCT/USE_ITHREADS/atarist/
      NO_ENV_ARRAY_IN_MAIN 等、このビルドでは常に偽になる分岐や
      `#ifndef PERL_MICRO`(常に真)のシグナル解除ループを削除。
      `perl_run()` の戻り値を `exitstatus` に反映するよう修正(miniperlmain.cは
      本来の perlmain.c と違い戻り値を握りつぶしていた)
      動作確認: `./romperl -e`, `test-float.pl`, `test-noproc.pl` 全て
      picoperl と同じ結果(バイナリサイズも 908,992 → 909,032 とほぼ同一)
* [x] read-only の ROMFS 形式を決める、互換性とシンプルを優先
      → 既存フォーマットとの互換は捨て、自作の最小フォーマットを採用
      (`romperl/romfs.h` に仕様を記述):
      - ヘッダ16byte(`magic[4]="RFS1"`, `file_count`, `total_size`, `reserved`)
        + エントリ40byte×file_count(`name[32]`, `offset`, `size`)+ ファイル
        データを連結しただけの単純な構造
      - ディレクトリ木は持たず、`name`はフルパス文字列("lib/feature.pm"のように
        先頭'/'なし)の完全一致検索のみ。readdir相当は無い
      - 整数はネイティブエンディアンのまま格納(x86_64ホストもCortex-M33
        ターゲットもどちらもリトルエンディアンなのでバイトスワップ不要という
        前提。他アーキテクチャに移植する場合は要見直し)
      - 圧縮なし、read-only専用
* [x] `mkromfs` で `root.romfs` を生成できるようにする
      → `romperl/mkromfs.pl`(開発ホストのシステムperlで実行するビルドツール。
      picoperl自身では動かさない)。`mkromfs.pl <src-dir> <out.romfs>` で
      ディレクトリを再帰的に拾って生成、`mkromfs.pl --list <image>` で
      中身を一覧できる検証モードも用意
      → `romperl/rootfs/`(`lib/feature.pm`のプレースホルダと`hello.pl`)から
      `root.romfs`を生成し、全ファイルのバイト列が元ファイルと一致することを
      手動検証済み。`root.romfs`はmkromfs.plから再生成可能な生成物なので
      `.gitignore`に追加(`rootfs/`ソース側のみコミット)
* [x] ROMFS を実行ファイルの `.romfs` セクションに組み込む
* [x] ROMFS を Flash 上から直接読めるようにする
      → この2項目はまとめて対応。読む側(`romfs_open`等)から見ればROMFSは
      「不変のバイト列へのポインタ」でしかなく、それがx86_64ではリンクされた
      `.romfs`セクション(プロセスのデータ領域)、Cortex-M33ではFlashに
      mmapされた領域、という違いはリンカ/ハードウェア側の話でAPI実装は
      共通化できる。`mkromfs.pl --carray`でrootfs/→root.romfs→Cの
      `unsigned char[]`(`__attribute__((section(".romfs")))`付き)まで生成
      するようMakefileに組み込んだ。`readelf -S`で`.romfs`セクションが
      実際に作られることを確認済み。Flash上でのmmap自体はRP2350実機が
      無いと検証できないため、Phase 6/7でのお楽しみとして残す
* [x] `open/read/seek/close/stat` の最小 API を実装する
      → `romperl/romfs.h`(宣言)+`romperl/romfs.c`(実装)。fdはROMFS_MAX_OPEN(8)個の
      固定配列から割り当て(動的確保なし)。`romfs_test.c`で13項目のスタンド
      アロンテストを書いて `make test-romfs` で実行できるようにした
      (Perl統合前にAPI単体の正しさを検証するため。ALL TESTS PASSED)
* [x] Perl のファイル I/O を ROMFS に接続する
* [x] `/lib/feature.pm` を ROMFS から `require/use` できるようにする
      → この2項目もまとめて対応。ユーザー指示により **picoperl-5.12.5/には
      一切手を入れず**、romperl/フォルダ内の新規ファイルだけで実現した:
      - `romperl/romfs_xs.c`: xsubppを使わず手書きしたXSUB。
        `Romperl::romfs_read(path)` (ROMFSの1ファイル全体をPerl文字列で返す)
        と、`Romperl::Boot::import` (下記フックをインストールする)の2つを
        `newXS()`登録
      - `require/use`をROMFSに繋ぐのはPerl標準の拡張ポイントである
        「`@INC`へのコードリファレンス登録」のみ。C側の`doio.c`/`perlio.c`
        は一切触っていない
      - `main.c`がargv先頭に`-MRomperl::Boot`を挿入することで、通常の
        `use`文と同じ安全なタイミング(BEGIN時)でフックが積まれるように
        した。**xs_init()から直接eval_pv()するとパーサ初期化前でクラッシュ
        する**ため、正規のuse/require経由の呼び出しにする必要があった
      - フック自体は「`lib/<filename>`をROMFSから読めれば返す、
        無ければ次の@INCエントリに委ねる」だけの3行のPerlコード
      - **ハマった点**: このpicoperlビルドは`useperlio='undef'`
        (USE_PERLIO無効、素のstdioのみ)のため、`open($fh,'<',\$scalar)`
        のin-memoryファイルハンドルが「Invalid argument」で失敗する
        (PerlIOの`:scalar`レイヤーが無い)。picoperl-5.12.5を再コンパイルして
        USE_PERLIOを有効化することはできないので、`pp_ctl.c`の`pp_require`
        (`PP(pp_require)`)がサポートするもう一つのプロトコル—フックが
        ファイルハンドルの代わりに「スカラーへのリファレンス1個」を返すと
        `filter_cache`としてソースフィルタ経由でそのまま読み込まれる—を
        使うことで解決。PerlIOを一切経由しない
      - 検証: `romperl/rootfs/lib/feature.pm`は本物(perl-5.12.5本体の
        `lib/feature.pm`をそのままコピー、プレースホルダではない)。
        `use feature 'say'; say 0.123;` がROMFS経由で実行でき、実際に
        `0.123`が出力されることを確認。ROMFSを持たないpicoperlでは
        `Can't locate feature.pm` で失敗することも確認
      - 参考: `use feature;`(引数無し)はfeature.pm自身の実装が
        `croak("No features specified")`するため`Carp.pm`が要る。
        `Carp.pm`はROMFSに入れていないため`Can't locate Carp.pm`で
        失敗するが、これはROMFS接続とは無関係のfeature.pm自体の仕様
* [x] テストファイルを作成しテストを組み込む
      → `romperl/test-inc-require.pl`(use経由のロード成功、`%INC`登録、
      存在しないモジュールは通常通り失敗して`@INC`探索が続くこと、の4項目)。
      `romperl/Makefile`に`test`ターゲットを追加し、`romfs_test`
      (API単体テスト)・`test-inc-require.pl`・`../test-float.pl`・
      `../test-noproc.pl`(picoperl向け回帰テスト)をまとめて実行できるように
      した。`make test`でPhase4全体の動作を一括確認できる

### Phase 4 追加: 依存モジュールの追加とゼロコピー化

- [x] `Carp.pm`/`strict.pm`/`warnings.pm`とその依存先を`romperl/rootfs/lib/`に
      追加(いずれもperl-5.12.5本体からの無改変コピー):
      - `Carp.pm` → `require Exporter` が必要 → `Exporter.pm`を追加
      - `Exporter.pm` → 一部の高度な機能で`Exporter::Heavy`を遅延require →
        `Exporter/Heavy.pm`を追加
      - `warnings.pm`のエラーパス(`Croaker`)は`Carp`を遅延require。
        `warnings::register`(`warnings/register.pm`)も併せて追加
        (`warnings.pm`にのみ依存する小さなモジュールなので)
      - 動作確認: `use strict; use warnings; use Carp;` に加えて
        `croak`/`carp`/`confess`(スタックトレース付き)まで実際に実行して
        正しい出力になることを確認。副次効果として`use feature;`
        (引数無し)も正しく`"No features specified"`とcroakするように
        なった(以前は`Carp.pm`が無く`Can't locate Carp.pm`で失敗していた)
- [x] `Romperl::romfs_read`をゼロコピー化(mallocの節約)
      - 設計: 返すSVの`SvPV`をmallocしたコピーではなく、ROMFSイメージ上の
        バイト列に直接向ける。`SvLEN(sv)=0`は「このSVはPVバッファを
        所有していない」という意味で、Perl内部の共有ハッシュキー文字列等
        と同じ表現。`sv.c`の`Perl_sv_clear`は`SvLEN`が0なら`Safefree`しない
        ため、このSVが解放されてもROMFS側のメモリを誤って解放することは
        無い(`sv.c:5864`で確認)
      - 制約(調査して判明): `require`/`use`の`@INC`フックが返す
        スカラーリファレンスは、`pp_ctl.c`の`S_run_user_filter`が
        1行読むごとに`sv_chop`で消費する。`sv_chop`(`sv.c:4707-4714`)は
        `SvLEN(sv)==0`のSVに対して「所有権の無い文字列のコピーを作る」
        処理を最初の呼び出し時に必ず行う(共有文字列を書き換え可能にする
        ための昇格)。つまり**require経由で読み込まれるモジュールは
        結局1回はコピーされる**(Perlの内部実装がそういう設計のため。
        picoperl-5.12.5には手を入れない方針なのでここは変更できない)
      - 得られる効果: (1) 完全に消費されない/require以外の用途で
        `Romperl::romfs_read`を使う場合(例えば将来スクリプトが
        データファイルを読むだけの用途)はmalloc/memcpyが一切発生しない。
        (2) requireの場合でも、コピーが「romfs_read呼び出し時」から
        「実際にsv_chopされる時」に遅延される。モジュールが最後まで
        使われる典型ケースでは総コピー量は変わらないが、無駄な早期
        コピーは無くなる
      - 動作確認: `make -C romperl test`全項目(ROMFS API単体13 + Perl統合
        4 + `test-float.pl`/`test-noproc.pl`回帰)がゼロコピー化後も
        パスすることを確認済み
      - **実測での検証**: 「余分な二重コピーが無いか」をソースコードの
        読解だけでなく実測でも確認した。`malloc`/`realloc`をLD_PRELOADで
        フックして閾値以上のサイズだけログするトレーサを作り、
        `use strict; use warnings; use Carp;` 実行時に各モジュールの
        ファイルサイズ(Carp.pm=16146, Exporter.pm=18494,
        warnings.pm=17476, strict.pm=3716 byte)に一致する大きさの
        `malloc`がそれぞれ**ちょうど1回ずつ**しか発生しないことを確認
        (2回以上出ればsv_chop昇格コピーとは別に余分なコピーが起きている
        ことになるが、そうはなっていない)。旧実装(romfs_read内で
        毎回mallocしていた版)でも同じ1回のコピーが発生するだけなので、
        今回の変更は「requireされて最後まで消費される」ケースでは
        コピー回数・総量ともに退行しておらず、それ以外の読み取り専用
        用途では完全にコピー0回になる、という設計通りの結果になっている

### Phase 4 追加: minify.pl で rootfs/lib/*.pm を圧縮生成

- [x] `minify.pl`(PPIでコメント/POD/空白を削るミニファイア、プロジェクト
      ルート)を使って`romperl/rootfs/lib/*.pm`を再生成するようにした
      - `romperl/rootfs/lib/*.pm`はもう手動コピーで git commit しない。
        `romperl/Makefile`に`rootfs/lib/%.pm: ../perl-5.12.5/lib/%.pm`
        というパターンルールを追加し、`perl ../minify.pl < 元ファイル >
        rootfs/lib/対象.pm`で毎回作り直す(root.romfs/romfs_data.cと同じ
        「生成物はコミットしない」扱いに統一。`.gitignore`に
        `/romperl/rootfs/lib/`を追加し、以前コミットしていた素コピーは
        `git rm --cached`した)
      - 効果: ROMFSイメージが69,460 → 22,955 bytes(約67%削減)。
        コメント・POD・空白を削ることでFlash容量を節約できる
      - **minify.plのバグを2件発見して修正**(いずれも「バスワード的な
        トークンの直後の空白を無条件に消す」ルールが、結合後に別の
        トークンとして再字句解析されてしまうケースを考慮していなかった):
        1. `eq`/`ne`/`lt`等の単語演算子の直後にWord/Numberが続く場合
           (例: `Carp.pm`の`ref $x eq ref \$i`)、空白を消すと
           `eqref`のように1つの識別子に結合されて構文エラーになる →
           後続の先頭文字が英数字/アンダースコア/単一引用符の場合は
           空白を残すよう修正
        2. bareword直後にQuoteが続く場合の`die"error"`最適化が、
           `warnings 'once'`のような単一引用符文字列で旧式パッケージ
           区切り記号(`Foo'bar`は`Foo::bar`と同義)と誤認識されたり
           (`Carp.pm`)、`qq[...]`のような英字始まりの引用形式で
           `warn`+`qq`が`warnqq`に結合されたりする(`Exporter/Heavy.pm`)
           → `"`または`` ` ``で始まる場合だけ空白を消すホワイトリスト
           方式に変更
      - 検証方法: 対象7ファイル(`strict.pm`/`warnings.pm`/
        `warnings/register.pm`/`Carp.pm`/`Exporter.pm`/
        `Exporter/Heavy.pm`/`feature.pm`)全てを`perl -c`で構文検証。
        さらに`perl-5.12.5/lib/`配下84個の`.pm`ファイル全部をminifyして
        `perl -c`にかける網羅テストも実施し、対象7ファイルは全て
        パスすることを確認
      - `make -C romperl test`全項目 + `croak`/`carp`/`confess`
        (スタックトレース付き)/`say`の統合確認、picoperl本体の
        回帰確認ともパス
      - 上記84ファイル網羅テストで、対象7ファイル以外に**ヒアドキュメント
        (`<<EOM`等)関連で8個中6個が失敗**していた: `deprecate.pm`
        `Dumpvalue.pm` `Benchmark.pm` `I18N/Collate.pm` `Getopt/Std.pm`
        `ExtUtils/Embed.pm`(残り2個は別の問題:
        `diagnostics.pm`は`my $_`絡みの意味論エラー、
        `Pod/Functions.pm`は`format`ブロック絡みで、いずれもヒアドキュメント
        とは無関係)。この時点では今回の対象ファイルに影響しないため
        未修正のまま残していた → 下記の通りCGI.pm追加時に修正した

- [x] `minify.pl`のヒアドキュメント破壊バグを修正し、`CGI.pm`
      (+依存の`CGI::Util`/`constant`/`overload`/`vars`)を追加
      - **バグの原因**: ヒアドキュメント(`<<'EOT'`等)は「呼び出し文の
        直後の物理行からボディが始まる」という前提で本文が保持される。
        従来のminify.plは「セミコロンの直後の空白/改行は消してよい」という
        ルールを無条件に適用しており、ヒアドキュメントを呼び出す文の
        直後の改行まで削除してしまっていた。結果、後続の文がヒアドキュメント
        呼び出し行に連結され(`my$x=<<'EOR';print"...";`のようになり)、
        本来の位置にあるべきボディの手前に後続コードが来てしまい、
        `Can't find string terminator` エラーになっていた
        (`CGI::Util.pm`の`<<'EOR'`で発覚、`perl-5.12.5/cpan/CGI/lib/CGI/Util.pm:252`)
      - **修正方針**: 安全に判定するコストが高いため、`PPI::Token::HereDoc`
        を含むファイルでは改行に触る2つの空白最適化ループ(演算子まわりの
        空白詰め、構造体まわりの空白除去)を丸ごと無効化するようにした
        (コメント/POD除去やqw()詰め等、改行を触らない最適化は従来通り適用)。
        安全性を優先し、ヒアドキュメントを含むファイルでは圧縮率が
        多少落ちるトレードオフを許容している
      - 再検証: 前述の6ファイル全て`perl -c`でsyntax OKになったことを確認。
        `perl-5.12.5/lib/`+`cpan/CGI/lib/`+`dist/constant/lib/`配下94個の
        `.pm`をminify+構文チェックする網羅テストを再実行し、新たな
        リグレッションが無いことを確認(残る失敗は`CGI.pm`本体や
        `CGI::Push`/`Switch`/`Pretty`/`Cookie`/`Apache`/`Fast`等の
        未対応サブモジュール群で、いずれも`perl -c`単体実行時の
        「依存モジュールが@INCに無い」という予期された失敗であり
        minifyのバグではない。`diagnostics.pm`/`Pod/Functions.pm`は
        引き続き別問題のまま未修正)
      - `CGI.pm`本体は`perl-5.12.5/cpan/CGI/lib/CGI.pm`(coreではなく
        cpanバンドル)。依存は`CGI::Util`(同ディレクトリ)、
        `constant`(`perl-5.12.5/dist/constant/lib/`)、
        `overload`/`vars`(`perl-5.12.5/lib/`、いずれも`warnings::register`
        にのみ依存)。`romperl/Makefile`にソースツリーの場所が異なる分の
        個別ルールを追加(`CPAN_CGI_LIB`/`DIST_CONSTANT_LIB`)
      - ROMFSサイズ: 22,955 → 160,496 bytes(CGI.pm本体が約26万byte→
        minify後も十数万byteある大きなモジュールのため)
      - 動作確認: `use CGI; CGI->new; escapeHTML; header;
        start_html/h1/end_html`、明示的なクエリ文字列を渡した
        `param()`取得まで実際に実行して確認。
        (`%ENV`経由の自動パラメータ取得は`$ENV{QUERY_STRING}`が
        空になり動かなかったが、これはromperl固有ではなくpicoperl自体が
        `%ENV`を全く populate しない既存の特性だった。CGI.pm追加や
        ROMFS接続とは無関係の別課題としてPhase 5以降で扱う)

### Phase 4 追加: pp_require を直接ROMFSに接続する true zero-copy化

現行の`@INC`フック方式(`Romperl::Boot`)は`\$src`(スカラーリファレンス)を
返し、`pp_require`はこれを`filter_cache`としてソースフィルタ経由で読む。
このためモジュール本体は`sv_chop`が初回に呼ばれた時点で結局1回コピーされる
(先の実測で確認済み)。5.12.5の`pp_ctl.c`を直接見ると、`@INC`探索を経ずに
`lex_start(line_sv, NULL, TRUE)` を呼べば、`line_sv`が
`SvREADONLY`でなく最後のバイトが`;`であれば`parser->linestr = line;`と
そのSVをコピーせず直接使う分岐がある(`toke.c`の`Perl_lex_start`、
`s[len-1] != ';'`チェック)。これを使い、`pp_require`自体をROMFSに
直接繋げばファイルサイズ分のmalloc/copyを完全にゼロにできる。

- [x] ROMFS側でファイルデータの直後にセンチネルを追加する
      (`mkromfs.pl`: 各ファイルの内容を書き込んだ直後に`";\0"`(2byte)を
      追加して`$data`に足す。`entry.size`は元のファイルサイズのまま
      変更しないので、通常の`stat`/`read`には影響しない)
      - 当初`;`1byteだけで実装したところ、後述の重大なクラッシュに
        遭遇し、`\0`も必須と判明したため2byteに変更した(詳細は下記)
- [x] `romfs.h`/`romfs.c`に`romfs_data_for_compile(path, &size)`を追加
      (通常の`romfs_data()`と違い、返す`size`にセンチネル`;`を含める
      = `entry->size + 1`。返り値のポインタ自体は同じ)
- [x] `romperl/romfs_compile.c`を追加し、`const void
      *romperl_find_for_compile(const char *name, unsigned long
      *out_len)` を実装。`"lib/<name>"`をROMFSから探し、見つかれば
      センチネル込みのポインタ/サイズを返す(`Romperl::Boot`の
      `"lib/$filename"`命名規則と揃える)
- [x] `picoperl-5.12.5/pp_ctl.c`の`pp_require`を`make-picoperl.sh`から
      sedパッチした(直接編集すると次回ビルドで消えるため、これまでの
      Phase2/3のパッチと同じ方式):
      1. `romperl_find_for_compile`の**weakなstub**(常にNULLを返す)を
         `pp_ctl.c`側に追加。`picoperl`はromperlのromfs_compile.oを
         リンクしないのでこのstubのままリンクされ、挙動は一切変わらない。
         `romperl`は同名の強いシンボルをromfs_compile.oからリンクし、
         リンカが強いシンボルを優先するので上書きされる
         (picoperlとromperlは`pp_ctl.c`由来の同じ`upp_ctl.o`を共有して
         いる=`romperl/Makefile`が.oをコピーせず参照する設計のため、
         コンパイル時の`#ifdef`分岐が使えない。weakシンボルによる
         リンク時のオーバーライドが唯一の現実的な手段)
      2. `pp_require`の変数宣言に`SV *romsv = NULL;`を追加
      3. `%INC`チェック直後・INC探索開始前に、
         `romperl_find_for_compile(name, &len)`を呼びROMFSに見つかれば
         `romsv`(`SvLEN=0`の所有権なしSV、`SvPV_set`/`SvCUR_set`で
         ROMFS上のバイト列に直接ポイント)を組み立てる
      4. 既存の`if (!tryrsfp) {`(2箇所: INC探索に入る条件と、
         見つからなかった時のDIE条件)を`if (!tryrsfp && !romsv) {`に
         変更し、ROMFSで見つかった場合は両方スキップする
      5. `lex_start(NULL, tryrsfp, TRUE);` を
         `lex_start(romsv ? romsv : NULL, tryrsfp, TRUE); if (romsv)
         SvREFCNT_dec(romsv);` に変更(lex_startが成功時に自分で
         refcountを+1するので、呼び出し側は借りた分を返す)
      - ハマった点: sedパッチのコメント文字列に生の`@INC`を書いたら、
        Perlの置換式の中で配列`@INC`として補間され、ビルド環境の
        システムperlの`@INC`一覧がコメントに埋め込まれてしまった
        (機能的には無害だが見苦しいので"INC"表記に直した。以後
        sedパッチのコメントに`@`から始まる語を書くときは要注意)
- [x] `romperl/Makefile`に`romfs_compile.o`のビルド・リンクを追加
      (perl.h不要なプレーンCなので`romfs.o`と同じ`$(OPTIMIZE)`のみで
      コンパイル可)
- [x] 動作確認: `picoperl`側は今まで通り全テストが通ること(weak stubが
      常にNULLを返すだけなので不変)を回帰確認。`romperl`側は
      `use feature 'say'; say 0.123;`、`use strict; use warnings; use
      Carp;`の`croak`/`carp`/`confess`、`use CGI;`の`escapeHTML`が
      引き続き動くこと、`test-inc-require.pl`/`test-float.pl`/
      `test-noproc.pl`が通ることを確認
- [x] **実測でのゼロコピー確認**: LD_PRELOAD mallocトレーサで
      `use strict; use warnings; use Carp;`実行時に、Carp.pm/Exporter.pm/
      warnings.pm/strict.pm等のいずれのサイズに一致する`malloc`/`realloc`も
      **1回も発生しない**ことを確認した(旧filter_cache方式では各ファイルに
      つき1回発生していたので、それがゼロになったことを実測で確認)。
      当初は使い捨てのデバッグツールだったが、再現・自動化できるよう
      `t/malloctrace.c`(トレーサ本体)+`t/test-zerocopy.sh`(検証スクリプト、
      `romperl/rootfs/lib/`配下の実際のファイルサイズを都度取得して
      malloc/reallocログと突き合わせる)としてコミットし、
      `make -C romperl test`の`test-zerocopy`ターゲットから自動実行される
      回帰テストにした(詳細は下記「テストを t/ に整理」参照)

### 実装中に遭遇した重大バグ: センチネル1byteだけでは不十分だった

`;`1byteだけをセンチネルとして実装した最初のバージョンで、
`require feature;`が`Out of memory!`で毎回クラッシュした。

- LD_PRELOADの`malloc`/`realloc`トレーサで実際に失敗している呼び出しを
  特定したところ、`realloc(ptr, 18446744073709551588)`
  (=`(size_t)-28`、符号なし整数のアンダーフロー)を発見
- gdbで`realloc`にサイズ引数の符号付き解釈が負になる条件でブレークし、
  バックトレースを取得: `Perl_sv_grow` ← `S_scan_str`(`toke.c`) ←
  `Perl_yylex` ← `Perl_yyparse` ← `S_doeval` ← `Perl_pp_require`
- `S_scan_str`(文字列/正規表現などクォート構文全般の字句解析、
  ヒアドキュメント固有ではない)に`fprintf`デバッグを仕込んで追跡した
  結果、**ファイル内最後の引用符構文を走査する際に、閉じ引用符が
  見つからないままバッファ終端(`PL_bufend`)を越えて走査し続ける**
  ことが判明。ミニファイル済み/生の`feature.pm`のどちらでも、常に
  ちょうど30バイト分バッファ終端を超えたところで見つかった
- 原因: PerlのSV文字列は`SvPVX(sv)[SvCUR(sv)]`(=長さのちょうど1つ先)
  が読める`'\0'`であることを内部の随所で前提にしている
  (`toke.c`の`S_scan_str`が閉じ引用符を探す際の境界チェック等)。
  センチネルを`;`1byteだけにしていたため、その1つ先の
  バイト(`SvCUR`の位置)は**次のROMFSエントリの中身**になっており、
  NULとは限らなかった。そのため閉じ引用符が見つからないまま
  隣のファイルのバイト列を読み進め、たまたまそこにあった`'`等の
  文字を閉じ引用符と誤認識するまで走査してしまい、結果として
  巨大な(不正な)サイズでの`realloc`要求につながっていた
- 修正: センチネルを`;`+`\0`の2byteに変更(`entry.size`はそのまま、
  `romfs_data_for_compile`が返す`size`も`entry.size+1`のまま
  変更不要。`\0`は`size+1`の"1つ先"に置かれ、報告されるサイズには
  含めないが物理的に必ず存在するようにした)
- 教訓: `lex_start()`のコピー回避条件(最後のバイトが`;`)だけでなく、
  Perl文字列APIの「SvCUR位置は読める'\0'」という**別の**暗黙の
  invariantも同時に満たす必要があった。ドキュメント化されていない
  内部の前提を壊さないよう、こうした最適化には実測での検証
  (今回はLD_PRELOADトレーサとgdbの条件付きブレークポイント)が
  欠かせない

- [ ] ヒアドキュメントを含む`.pm`をrequireした場合の動作確認はまだ
      していない。上記の`\0`修正で境界超え自体は解消したはずだが、
      ヒアドキュメント特有の「バッファを書き換える」処理
      (`sv_chop`の昇格と同じ理屈で`SvLEN=0`から自動的にオウンド
      コピーへ昇格する想定)が実際にクラッシュしないかは別途確認が要る
- [ ] `__DATA__`は`PL_rsfp`(このパスでは常にNULL)を前提に
      `Package::DATA`ハンドルが作られる実装のため、この方式では
      `__DATA__`を持つモジュールは非対応になる。明示的にドキュメント化
      する(現時点でROMFSに入れているモジュールはどれも`__DATA__`を
      使っていないので実害なし)
- [ ] 上記が安定したら、既存の`Romperl::Boot`(`@INC`フック)は
      直接パスがカバーする範囲では二度と呼ばれなくなり冗長化する。
      当面はフォールバックとして残すか、削除して一本化するかを判断する

### Phase 4 追加: テストを t/ フォルダに整理し、mallocトレーサをコミット

これまでテストファイルはプロジェクトルート(`test-float.pl`/
`test-noproc.pl`)と`romperl/`(`test-inc-require.pl`/`romfs_test.c`)に
分散していた。また実測でのゼロコピー検証に使ったLD_PRELOAD mallocトレーサは
使い捨てで`/tmp`に置いたままコミットしていなかった。再現性を上げるため
`t/`フォルダに集約し、mallocトレーサも回帰テストとしてコミットした。

- [x] `test-float.pl`/`test-noproc.pl`(プロジェクトルート)と
      `romperl/test-inc-require.pl`/`romperl/romfs_test.c`を`t/`に
      `git mv`で移動。`romfs_test.c`は`romfs.h`(`romperl/`側)に依存するため、
      `romperl/Makefile`のビルドルールに`-I.`を追加してカレントディレクトリ
      (`romperl/`)をinclude pathに含めるよう修正(`t/`から見た相対パスでは
      `"romfs.h"`を解決できないため)
- [x] `make-picoperl.sh`末尾の`./picoperl ../test-float.pl`等と、
      `romperl/Makefile`の`test`/`test-inc-require`ターゲットを、
      それぞれ`../t/test-float.pl`等の新しいパスに更新
- [x] `t/malloctrace.c`としてLD_PRELOAD mallocトレーサをコミット
      (`malloc`/`realloc`を閾値`MALLOCTRACE_MIN`(デフォルト64byte)以上の
      サイズだけログし、`realloc`のサイズが符号付き解釈で負になる異常
      (センチネル不足によるバッファ境界超え等)は`UNDERFLOW`として別枠で
      警告する。まさにこの仕組みの原型で`;`1byteセンチネル不足バグを
      発見した)
- [x] `t/test-zerocopy.sh`としてゼロコピー検証を自動化。
      `malloctrace.so`をビルドし、`romperl -e 'use strict; use warnings;
      use Carp;'`を実行、`romperl/rootfs/lib/`配下の実ファイルサイズ
      (ビルドの度にminify.plの出力次第で変わりうるため固定値にせず
      都度`wc -c`で取得)に一致する`malloc`/`realloc`が無いこと、および
      `UNDERFLOW`ログが無いことを確認する
- [x] `romperl/Makefile`に`test-zerocopy`ターゲットを追加し、`test`
      ターゲット(`test-romfs`/`test-inc-require`/`test-float.pl`/
      `test-noproc.pl`と並ぶ形)から自動実行されるようにした。
      `clean`ターゲットで`malloctrace.so`(生成物)も削除する
- [x] `.gitignore`に`*.so`を追加(`malloctrace.so`はビルドの度に
      作り直す生成物としてコミットしない)
- [x] 動作確認: `make -C romperl test`一式(`romfs_test`13項目 +
      `test-inc-require.pl`4項目 + `test-zerocopy.sh`(strict/Carp/
      Exporter/Exporter::Heavy/warnings/warnings::register の6項目) +
      `test-float.pl`/`test-noproc.pl`回帰)が新しいパス構成で
      全てパスすることを確認。`./make-picoperl.sh`単体(`t/test-float.pl`/
      `t/test-noproc.pl`)も引き続きパス

## Phase 5 準備: 簡易 RAMFS (書き込み可能インメモリファイルシステム)

romfs(romperl/romfs.h)はFlashに埋め込む読み取り専用イメージで、実行時の
ファイル新規作成・更新・削除ができない。Phase 5の「ファイル系をROMFS前提に」
「stdioをPerlIO経由でUARTに直結」より前に、libcの`fopen`系の裏側で動く
書き込み可能なインメモリファイルシステムを単体で用意しておく。

方針(ユーザー指示どおり):
- POSIX互換は目指さない。API名/引数はfopen/fread/fwrite/fseek/ftell/
  fclose/removeに合わせるが、意味論は最低限(モード文字列は`r`/`w`/`a`/
  `r+`/`w+`/`a+`のみ解釈、`b`は無視)
- 性能よりシンプルさ優先。事前の大きなメモリ確保はせず、書き込みの都度
  必要な分だけmalloc/reallocに頼ってよい(倍々確保などの最適化はしない)
- まだPerl/PerlIOには組み込まず、Cだけで作成・更新・削除を単体テスト
  できるようにする

- [x] `ramfs/ramfs.h` / `ramfs/ramfs.c` を新規作成
      - データ構造: ファイル数上限を決め打ちしないフラットな単方向リスト
        (`struct ramfs_inode { name, data, size, next }`)。romfsと同じ
        フラットな名前空間(ディレクトリ階層なし、名前は31文字+NUL)
      - `ramfs_fopen(name, mode)`: `w`は無ければ新規作成・あれば`trunc`、
        `a`は末尾に追記(`fseek`で動かしても書き込みは常に末尾)、
        `r+`は既存ファイルの部分上書き。無いファイルを`r`/`r+`で開くと
        `NULL`
      - `ramfs_fwrite`: 書き込み範囲が現在のsizeを超える時だけ
        `realloc`でちょうど必要な分だけ伸ばす。`fseek`で末尾より先に
        飛んでから書く(sparse write)と、間の隙間は`memset`でゼロ埋め
        する
      - `ramfs_remove(name)`: リストから外して`data`/inode本体を`free`。
        既知の制約として、削除対象を指す`ramfs_FILE*`を別途開いたまま
        削除すると、その後のアクセスは未定義動作になる(参照カウント
        等の安全策は「シンプルさ優先」の方針により実装していない)
      - `ramfs_init()`: 全ファイルを解放して空の状態に戻す
        (起動時初期化やテストのリセット用)
- [x] `t/ramfs_test.c` を新規作成し、Cから単体で動作確認
      - 新規作成(`w`)・読み取り(`r`)・部分上書き(`r+`)・
        トランケート再作成(`w`で開き直す)・追記(`a`/`a+`、seekしても
        常に末尾に書かれること)・`fseek`(SET/CUR/END、負値は失敗)・
        sparse writeのゼロ埋め・削除(`remove`、削除後は`r`で開けない/
        `stat`も失敗する/同名で再作成できる)・名前長すぎエラー・
        `ramfs_init`での全消去、の29項目
      - `ramfs/Makefile`に`test`ターゲットを追加(`make -C ramfs test`)。
        `-Wall -Wextra -Werror`で警告0件を確認
      - 動作確認: 29項目全てパス。まだpicoperl/romperlには未接続
- [x] romfsとramfsの統合方針を決め、`vfs/`ディスパッチ層として実装
      (詳細は下記「romfsとramfsを統合するvfsディスパッチ層」参照)

### Phase 5 準備: romfsとramfsを統合するvfsディスパッチ層

romfs(読み取り専用)とramfs(書き込み可能)をPerl/PerlIOから見て1本の
ファイルアクセスに見せるための方針をユーザーと相談して決定し、
`vfs/vfs.h` / `vfs/vfs.c` として実装した。romfs.c/ramfs.cの実装自体には
一切手を入れず、`vfs_fopen`等がどちらを呼ぶか振り分けるだけの薄い層。

検討した2案:
1. **単純な使い分け(採用)**: 書き込みを伴うopenは常にramfsだけを対象に
   する(romfsには一切触れない)。読み取り専用openはramfs優先→romfs
   フォールバック。名前衝突時の挙動(後述のshadow/resurrect)は
   「未定義/保証しない」と割り切る
2. OverlayFS方式(whiteout付き): romfs上のファイルをremove/上書きした
   ように見せるため、削除マーカーやcopy-up処理を実装する案。
   POSIX的な直感には合うが実装・テストの複雑度が増えるため見送った

- [x] `vfs/vfs.h` / `vfs/vfs.c` を新規作成
      - API: `vfs_fopen`/`vfs_fread`/`vfs_fwrite`/`vfs_fseek`/`vfs_ftell`/
        `vfs_feof`/`vfs_fclose`/`vfs_remove`/`vfs_stat`/`vfs_init`
        (ramfs.hのfopen系命名にそのまま合わせている)
      - `vfs_fopen`: modeが書き込みを含む(`w`/`a`/どこかに`+`)場合は
        `ramfs_fopen`のみを呼ぶ。読み取り専用(`r`)の場合は
        `ramfs_fopen(name,"r")`を先に試し、失敗したら`romfs_open(name)`に
        フォールバックする
      - **shadowingは特別な実装をせずに自然に得られる**: 「読み取りは
        ramfs優先」というルールだけで、romfsと同名のファイルにwriteすれば
        以後の読み取りはramfs側の内容が見える(copy-up不要)
      - `vfs_remove`はramfsだけを対象にする(romfs上のファイルは削除
        できない)。ramfs側にshadowが無い状態でromfs専用ファイルを
        removeしようとすると失敗する(`-1`)
      - **既知の割り切り**: shadow(ramfs側の上書きコピー)を`remove`した
        場合、whiteoutを実装していないため以後の読み取りはromfsの
        元の内容に「戻る」。この程度の衝突時の挙動は保証しないという
        方針(romfsに入れているのはビルド時埋め込みの固定モジュールだけ
        なので実用上は問題にならない想定)
      - `vfs_init()`はramfsだけをリセットする(romfsイメージの寿命管理は
        呼び出し側の責務のまま)
- [x] `t/vfs_test.c` を新規作成し、Cから単体で動作確認
      - 未知の名前(失敗)、romfsのみ(フォールバックで読める/書き込みは
        拒否される)、ramfsのみ(新規作成・r+更新)、shadow(romfsと同名を
        writeすると以後ramfs優先で読める)、shadow削除後の
        resurrect(romfsの内容に戻る)、shadowされていないromfs専用
        ファイルのremove失敗、ramfs専用ファイルの通常remove、
        `vfs_init`がramfsだけをリセットしromfsに影響しないこと、
        の21項目
      - `vfs/Makefile`に`test`ターゲットを追加(`make -C vfs test`)。
        テスト用のromfsイメージは`vfs/testdata/readonly.txt`1つだけを
        `romperl/mkromfs.pl`(フォーマット共通のため生成ツールも共有)で
        埋め込んで使う。`-Wall -Wextra`で警告0件を確認
      - 動作確認: 21項目全てパス。まだpicoperl/romperlには未接続
- [x] ついでに`romperl/romfs.h`のフォーマット説明コメントが、
      センチネルを`;`1byteだけと記述したまま(実装済みの`;`+NUL 2byte
      修正が反映されていなかった)古くなっていたのを修正した
- [ ] Perl/PerlIOへの実接続はまだ行っていない。Phase 5の「ファイル系を
      ROMFS前提に」「stdioをPerlIO経由でUARTに直結」に着手する際に、
      libc-pico2のstdioシムから`vfs_fopen`系を呼ぶようにする想定。
      `pp_require`の直結zero-copyパス(romfsのみ見る)は変更不要のまま
      残し、そこでromfsに見つからなかった場合の`@INC`探索フォールバック
      (通常の`fopen`相当)が将来この`vfs_fopen`経由になれば、実行時に
      ramfsへ書かれたモジュールも`require`できるようになる副次効果が
      見込める

## Phase 5: libc 依存の削減 → libc-pico2/ フォルダで *.h *.c を作成

- [x] `#include <*.h>` で `../libc-pico2/` フォルダが優先されて読み込まれるように
      → `make-picoperl.sh` の `OPTIMIZE` に `-I../libc-pico2` を追加を確認
- [x] プロセス系を無効: `fork` `execl` `execv` `execvp` `wait` `kill` `pipe`
      `sleep` `getpid` `getuid` `geteuid` `getgid` `getegid` `setuid` `setgid`
      (主に `pp_sys.c` / `doio.c`) → `libc-pico2/unistd.h` / `signal.h` /
      `sys/wait.h` で常にエラー(`errno=ENOSYS`)または固定値を返すマクロに
      置き換えた。`test-noproc.pl` を追加して `make-picoperl.sh` から自動実行。
      `nm -u` の未定義シンボル数: 104 → 88 個
  - `libc-pico2/unistd.h`: `fork`/`execl`/`execv`/`execvp`/`pipe`/`sleep`/
    `getpid`(→1)/`getuid`/`geteuid`/`getgid`/`getegid`(→0)/`setuid`/`setgid`
  - `libc-pico2/signal.h`: `kill`/`killpg`
  - `libc-pico2/sys/wait.h`: `wait`/`waitpid`
  - ハマった点1: `perl.h` が `Uid_t getuid (void);` のように getuid 等を
    無条件に素のプロトタイプ再宣言している。関数マクロはコール式だけでなく
    宣言文中の同名トークンも展開してしまい `Uid_t ((uid_t)0);` のような
    壊れた宣言になってコンパイルエラーになった。可変引数マクロ
    (`#define getuid(...) ...`)にしても「宣言と展開後の構文が合わない」
    問題自体は解決しないため、結局 `perl.h` 側のその宣言ブロックを
    `#ifndef PICOPERL_LIBC_PICO2_UNISTD_H` で無効化する追加パッチが必要だった
  - ハマった点2: `i_syswait='undef'` のため `<sys/wait.h>` がどこからも
    include されておらず、`wait()` が暗黙宣言のまま本物のlibc関数を直接
    呼んでいた(`libc-pico2/sys/wait.h` を置いても include されなければ
    差し替わらない)。`uconfig.sh` の `i_syswait` を `define` にして
    `#include <sys/wait.h>` を実際に通るようにして解決
  - `kill(0,$$)` は `libc-pico2/signal.h` の前に `doio.c` の `apply()` が
    `#ifndef HAS_KILL` で "The kill function is unimplemented" と die する
    既存の仕組みが先に効いていた(`d_kill='undef'` のため)。signal.h の
    シムは `apply()` を経由しない直接呼び出し経路への保険として残す
- [x] `floor`/`ceil`/`fmod`(double版)が `floorf`/`ceilf`/`fmodf` と両方リンク
      されている件を調査(`nm -u picoperl`で両方確認: `floor`/`ceil`は
      `@GLIBC_2.2.5`、`fmod`は`@GLIBC_2.38`、`floorf`/`ceilf`/`fmodf`も別途
      リンクされている)
      - 直接呼び出し箇所を`grep`で特定した結果、`Perl_floor`等のマクロ
        (NVSIZE==4分岐で`floorf`等を使う、Phase2で対応済み)を経由しない
        直接呼び出しは**`time64.c`だけ**だった(`pp_pack.c`/`numeric.c`には
        無かった。TODOの元の想定は不正確だった):
        `S_gmtime64_r`内で`v_tm_sec`/`v_tm_min`/`v_tm_hour`/`v_tm_wday`を
        `fmod(time, 60.0)`等で、うるう年サイクル数を`floor`/`ceil`で計算
      - **float版に統一すべきではないと判断、修正不要でクローズ**:
        `time64.c`のこれらの計算はPerlのNV(スカラー浮動小数点数)とは
        無関係の、`Time64_T`(`typedef Int64 Time64_T`、64bit整数)で
        保持するUnixエポック秒を暦(年月日時分秒)に分解するための
        C内部の整数演算。現在のエポック秒(約17億)はfloatの仮数部が
        正確に表現できる整数の上限(2^24=16,777,216)を遥かに超えており、
        floatにしてしまうと時刻計算そのものが壊れる。doubleの仮数部
        (2^53)なら余裕で収まるため、ここは意図的にdoubleが必要な箇所
      - 結論: `floor`/`ceil`/`fmod`のdouble版とfloat版が両方リンクされる
        状態は正しい仕様であり、Perlスカラーの数値計算(`Perl_floor`等)は
        既にfloat版を使っている。統一は不要、以後この項目は再検討しない
- [x] ファイル系を ROMFS/RAMFS 前提に(`stat`/`unlink` のみ完了、
      `open`/`close`/`read`/`write`/`lseek`/`fstat`は次項と統合して保留。
      `opendir`/`readdir`/`closedir`/`chdir`/`chmod`/`rename`/`umask`/
      `dup`/`isatty`/`tmpfile`は未着手)
      - `libc-pico2/sys/stat.h`(新規)で`stat()`(パス名ベース)を
        `picoperl_stat()`に、`libc-pico2/unistd.h`で`unlink()`を
        `picoperl_unlink()`にリダイレクト。実体は`posix_shim.c`
        (project root、`make-picoperl.sh`が`picoperl-5.12.5/`にコピーし
        `Makefile`に`uposix_shim$(_O)`としてビルドルールを追加)の
        `__attribute__((weak))`な既定実装(本物のシステムコールへの
        パススルー、plain picoperlの挙動は不変)と、`vfs/vfs_posix.c`
        の強い実装(`vfs_stat`/`vfs_remove`経由、romperlがリンク時に
        上書き)の組。pp_requireの`romperl_find_for_compile`と同じ
        「弱いデフォルト+強い上書き」の仕組みを再利用した
      - 動作確認(romperl): `-e "lib/feature.pm"`/`-s`が実ファイルの
        無いROMFS埋め込みモジュールに対して正しくyes/サイズを返す
        ことを確認(ホストの実ファイルシステムには存在しないことを
        `ls`で確認済み)。`unlink("lib/feature.pm")`(romfs専用)は
        `vfs_remove`がramfsしか見ないため正しく失敗しファイルは残る。
        `t/vfs_posix_test.c`(`vfs/Makefile`の`test`に追加、10項目)で
        romfsフォールバック/ramfs優先/削除可否をC単体でも検証
      - 動作確認(plain picoperl): 実ファイルに対する`-e`/`-s`/`unlink`が
        今まで通り動くこと(弱いデフォルトが本物のシステムコールへ
        委譲するだけなので無変更)を確認。バイナリサイズも変更前と
        完全に同一(908,992 bytes)
      - **重大な発見: `open`/`sysopen`は単独ではリダイレクトできない**。
        実装当初`open`/`close`/`read`/`write`/`lseek`も同じ弱い/強い
        方式で`vfs_posix.c`にリダイレクトしたところ、romperlで
        `sysopen`が`Bad file descriptor`で必ず失敗した。原因を追跡
        (`perlio.c`の`PerlIOStdio_openn`)した結果、このビルド
        (`useperlio='undef'`)では`sysopen`が`PerlLIO_open3()`
        (=リダイレクトした`open()`)で得たfdを**直後に本物の
        `PerlSIO_fdopen()`(=fdopen)へ渡してFILE*化する**ことが判明。
        vfsの偽fd(実OSのfdではない)を本物の`fdopen()`に渡せば
        `EBADF`になるのは当然で、`open()`だけをvfsにリダイレクトしても
        機能しない。`fdopen`/`fopen`以降のstdio全体(`fread`/`fwrite`/
        `fclose`/`fseek`等、次のTODO項目そのもの)を同時にvfs対応
        させない限り、`open`/`close`/`read`/`write`/`lseek`(と、
        vfs経由のfdが存在しないと無意味な`fstat`)は動かせないと判断し、
        `libc-pico2/fcntl.h`(削除)・`vfs_posix.c`の該当実装を全て
        引き上げ、`stat`/`unlink`(パス名だけで完結しfd/FILE*を経由
        しない)だけを残した
      - この発見により、TODOの元の想定(「ファイル系をROMFS前提に」→
        その後「stdioをPerlIO経由でUARTに直結」という順番)は誤りだった
        と判明。この2項目はuseperlio=undefである限り**分離できない**
        (sysopenが常にfdopenを経由するため)。次にstdio項目に着手する
        際、`open`/`close`/`read`/`write`/`lseek`/`fstat`もそこで
        まとめて対応すること
- [x] fopen系(`fopen`/`fclose`/`fread`/`fwrite`/`fseek`/`ftell`/`feof`/
      `ferror`/`clearerr`/`fflush`/`fgetc`/`fputs`/`fileno`/`fprintf`)を
      vfs(romfs+ramfs)経由にリダイレクト。`open`/`close`/`read`/`write`/
      `lseek`/`fstat`(POSIXの生fd系)・`fdopen`/`ungetc`/`tmpfile`は対象外
      (ユーザー指示: 「自作のlibcに置き換えた際に呼ばれることがなければ、
      POSIX系のopen/closeは一切実装しなくて良い。libcのfopen系だけの
      対応で良い」)
      - **鍵となる発見**: このビルド(`useperlio=undef`)では`PerlIO`が
        `#define PerlIO FILE`されており(`perlsdio.h`)、Perlの通常の
        `open()`は`#define PerlIO_open PerlSIO_fopen`により**`fopen()`に
        直結**していて、POSIXの生fd(`open`/`close`/`read`/`write`/
        `lseek`)や`fdopen()`を一切経由しない。これらを経由するのは
        `sysopen`(数値モードのopen)だけであり、それは対象外として
        割り切れる、という設計の裏付けが取れた
      - `libc-pico2/stdio.h`(新規)で各関数をリダイレクト。実体は
        `stdio_shim.c`(project root、`posix_shim.c`と同じ手順で
        `make-picoperl.sh`が`picoperl-5.12.5/`にコピーし
        `ustdio_shim$(_O)`としてビルド)の弱いデフォルト実装と、
        `vfs/vfs_stdio.c`の強い実装(romperlがリンク時に上書き)の組
      - **FILE\*の扱い**: `picoperl_fopen()`が返す`FILE*`はglibc本物の
        構造体ではなく、`vfs_FILE*`をラップしmagic numberを付けた
        独自構造体へのポインタ(呼び出し側は常にこのヘッダ経由の
        関数だけでオペークに扱うので実際のレイアウトの違いは問題に
        ならない)。`fread`/`fwrite`/`fclose`等は渡された`FILE*`が
        このmagicを持つか調べ、持たなければ「本物のFILE\*」
        (stdin/stdout/stderr等)とみなして本物のfread/fwrite/fclose等に
        そのまま委譲する(ポインタ先頭1ワードだけを見る割り切った
        判定だが、本物のglibc FILE構造体の同オフセットが偶然
        一致する可能性は無視できるほど低い)
      - **重大な追加発見(実装中に判明)**: `fopen()`を無条件にvfsへ
        リダイレクトすると、**romperl自身がPerlスクリプトファイルを
        コマンドライン引数で受け取って開く経路**(perl.cのスクリプト
        読み込みも同じ`fopen()`を通る)まで巻き込んでしまい、
        `./romperl foo.pl`が常に`Can't open perl script`で失敗する
        重大な回帰を引き起こした。修正として、`picoperl_fopen()`は
        読み取り専用(`"r"`、`+`を含まない)でvfsに見つからない場合に
        限り、開発ホストの実ファイルシステムへの`fopen()`にフォール
        バックするようにした。書き込みを伴うモードは`vfs_fopen`が
        ほぼ確実に成功するためこのフォールバックには来ない(=vfsの
        書き込みは常にvfs専用という方針は崩れない)。実機(RP2350、
        実ファイルシステム自体が存在しない)ではこの分岐は単に
        失敗するだけで無害
      - 動作確認: `t/vfs_stdio_test.c`(C単体、21項目、`make -C vfs test`)
        + `t/test-stdio.pl`(Perl統合、`open`/`print`/`close`/`readline`/
        追記/`printf`/ROMFS埋め込みファイルの直接open、5項目、
        `make -C romperl test`の`test-stdio`ターゲット)。`made.txt`等が
        ホストの実ファイルシステムには一切書かれないことも確認。
        plain picoperlは無変更(既存回帰テスト全てパス)
- [ ] 環境変数の実装: `getenv` `putenv` → freeしないハッシュに格納する
      (Phase4で判明: picoperlは現状`%ENV`を全く populate しない。
      `QUERY_STRING="..." ./picoperl -e 'print $ENV{QUERY_STRING}'`が
      空になることを確認済み。CGI.pmの自動パラメータ取得等に影響する)
- [ ] `qsort` → `pp_sort.c` 内製ソートに寄せる
- [ ] `rand` / `srand` → 内製 PRNG に置き換え
- [ ] `localtime` / `time` → `time64.c` + 固定エポックの時刻を返す
- [ ] `malloc` / `calloc` / `realloc` / `free` → 固定ヒープアロケータ
- [ ] `__ctype_b_loc` (locale 依存) を外す → `locale.c` の除去とセット
- [ ] `setjmp` / `longjmp` は Cortex-M33 でも必要。まずは x86_64 で。

## Phase 6: ARM Linux/Thumb で中間検証

- [ ] `arm-linux-gnueabihf-gcc -mthumb` でクロスビルドを試す、テストはまだ
- [ ] `uconfig.sh.pico2` ファイルを作り、コピー処理を `make-picoperl.sh` の
      `make regen_uconfig` に追加する。クロスコンパイルのオプション pico2 を
      追加した場合このコピー処理を実行してCCも変更する
      クロスコンパイルしない場合も考慮していままでの処理は残す
- [ ] `qemu-arm` 上で `test-float.pl` を通すようにする

## Phase 7: arm-none-eabi / Cortex-M33 (RP2350)

- [ ] `arm-none-eabi-gcc -mcpu=cortex-m33 -mthumb` で動くように実装
- [ ] リンカスクリプト / スタートアップ / スタックサイズの決定
- [ ] ヒープサイズと RP2350 の RAM (520KB) に収まるかの見積もり
- [ ] `setjmp`/`longjmp` と例外処理の動作確認

## Phase 8: RAMFS + UART のみで動作

- [ ] バイナリに埋め込まれた ROMFS と RAMFS をマージ実装
- [ ] PerlIO を UART ドライバに接続 (stdin/stdout/stderr のみ)
- [ ] QEMU libvirt で起動確認

## 検証コマンド（必要に応じて追加）

```sh
./make-picoperl.sh
cd picoperl-5.12.5
./picoperl -e 'print 0.123, "OK\n"'
./picoperl ../t/test-float.pl
nm -u picoperl | wc -l
wc -c < picoperl
```

テスト一式は `t/` フォルダに集約している(`test-float.pl`/`test-noproc.pl`/
`test-inc-require.pl`/`romfs_test.c`/`malloctrace.c`/`test-zerocopy.sh`)。
`romperl` 側は `make -C romperl test` で一括実行できる。
