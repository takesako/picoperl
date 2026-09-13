# TODO

picoperl: microperl を RP2350 (Cortex-M33) 向け最小 Perl にする作業リスト。
詳細な方針は [CLAUDE.md](CLAUDE.md) を参照。

## 現状 (2026-09-13, x86_64 WSL/Debian)

- `./make-picoperl.sh` でビルド成功、`picoperl -e` 動作確認済み
- バイナリサイズ: 908,992 bytes (`-Os -flto -ffunction-sections -fdata-sections` + `--gc-sections`)
- NV は float 化済み: `nvtype='float'` / `nvsize='4'` / `ivsize='8'`
- `NV_DIG`/`NV_MANT_DIG`/`NV_MIN`/`NV_MAX`/`NV_EPSILON` も `FLT_*` 基準に修正済み
  (Phase 3)。数値の文字列化(`print`/`sprintf`のデフォルト精度)が float 相当になった
- `./picoperl test-float.pl` は `# NV=float` + `ALL TESTS PASSED`(stringify precision
  テストを追加)
- `libc-pico2/` フォルダ(Phase 5)で `fork`/`exec*`/`pipe`/`kill`/`wait*`/`sleep`/
  `get{u,g,eu,eg}id`/`set{u,g}id` を無効化・固定値化。`./picoperl test-noproc.pl`
  も `ALL TESTS PASSED`
- 未定義 libc シンボル: 88 個 (`nm -u picoperl`, Phase 1時点は96個)。`floor`/`ceil`/
  `fmod` は double 版もまだ別箇所から直接呼ばれており float 版と両方リンクされて
  いる(Phase 5 で要精査)
- `romperl/`(Phase 4)完成: `../picoperl-5.12.5`の`.o`を参照するだけで
  picoperl-5.12.5自体には一切手を入れずに、自作ROMFS形式の埋め込み
  (`.romfs`セクション)、`open/read/seek/close/stat`最小API、
  `@INC`フック経由の`require/use`接続まで実装。`make -C romperl test`で
  一括検証できる(`romfs_test`のAPI単体テスト13項目 +
  `test-inc-require.pl`のPerl統合4項目 + `test-float.pl`/`test-noproc.pl`の
  回帰確認)

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
        パスすることを確認(残り8個はヒアドキュメント`<<EOM`関連の
        別種の未修正の問題で失敗するが、今回の対象ファイルには
        含まれないため未対応のまま。ヒアドキュメントを含むファイルを
        将来ROMFSに入れる場合は要注意)
      - `make -C romperl test`全項目 + `croak`/`carp`/`confess`
        (スタックトレース付き)/`say`の統合確認、picoperl本体の
        回帰確認ともパス

## Phase 5: libc 依存の削減 → libc-pico2/ フォルダで *.h *.c を作成

- [ ] `#include <*.h>` で `../libc-pico2/` フォルダが優先されて読み込まれるように
      → `make-picoperl.sh` の `OPTIMIZE` に `-I../libc-pico2` を追加を確認
- [ ] プロセス系を無効: `fork` `execl` `execv` `execvp` `wait` `kill` `pipe`
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
- [ ] `floor`/`ceil`/`fmod`(double版)が `floorf`/`ceilf`/`fmodf` と両方リンク
      されている(`nm -u` で確認)。`Perl_floor`等のマクロ経由以外にも
      `pp_pack.c`/`numeric.c`/`time64.c` あたりで直接 `floor()` 等を呼んでいる
      箇所がある想定。float 版に統一できるか、struct tm 計算など double が
      本質的に必要な箇所かを切り分ける
- [ ] ファイル系を ROMFS 前提に: `open` `close` `read` `write` `lseek` `stat`
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

## Phase 6: ARM Linux/Thumb で中間検証

- [ ] `arm-linux-gnueabihf-gcc -mthumb` でクロスビルドを試す、テストはまだ
- [ ] `uconfig.sh.pico2` ファイルを作り、コピー処理を `make-picoperl.sh` の
      `make regen_uconfig` に追加する。クロスコンパイルのオプション pico2 を
      追加した場合このコピー処理を実行してCCも変更する
      クロスコンパイルしない場合も考慮していままでの処理は残す
- [ ] `qemu-arm` 上で `test-float.pl` を通すようにする

## Phase 7: arm-none-eabi / Cortex-M33 (RP2350)

- [ ] `arm-none-eabi-gcc -mcpu=cortex-m33 -mthumb` + newlib-nano を参考に実装
- [ ] リンカスクリプト / スタートアップ / スタックサイズの決定
- [ ] ヒープサイズと RP2350 の RAM (520KB) に収まるかの見積もり
- [ ] `setjmp`/`longjmp` と例外処理の動作確認

## Phase 8: RAMFS + UART のみで動作

- [ ] スクリプトをバイナリに埋め込む RAMFS を実装
- [ ] PerlIO を UART ドライバに接続 (stdin/stdout/stderr のみ)
- [ ] QEMU libvirt で起動確認

## 検証コマンド（必要に応じて追加）

```sh
./make-picoperl.sh
cd picoperl-5.12.5
./picoperl -e 'print 0.123, "OK\n"'
./picoperl ../test-float.pl
nm -u picoperl | wc -l
wc -c < picoperl
```
