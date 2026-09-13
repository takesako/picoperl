/*
 * ROMFS を Perl の @INC フック経由で require/use に繋ぐための最小限の
 * XSUB。picoperl-5.12.5 側 (doio.c/perlio.c) には一切手を入れず、Perl
 * 標準の拡張ポイントである「@INCへのコードリファレンス登録」だけで
 * 完結させる。C側で用意するのはROMFSからファイル全体を読んで
 * Perl文字列として返す関数と、@INCフック本体を仕込む
 * "Romperl::Boot" 仮想モジュールのimport()だけ。
 *
 * このpicoperlビルドは useperlio='undef' (USE_PERLIO 無効、素のstdioのみ)
 * なので、open($fh,'<',\$scalar) のようなin-memoryファイルハンドルは
 * "Invalid argument" で失敗する(PerlIOの:scalarレイヤーが無いため)。
 * picoperl-5.12.5を再コンパイルしてUSE_PERLIOを有効化することはできない
 * (romperlは.oを参照するだけでpicoperl側は変更しない方針のため)ので、
 * pp_ctl.cのpp_require実装がサポートしているもう一つのプロトコル
 * ―フックが「ファイルハンドルではなくスカラーリファレンス1個」を
 * 返した場合、その中身をソースフィルタ(filter_cache)として直接
 * 読み込む―を使う。PerlIOを一切経由しないので、このビルドでも動く。
 *
 * 注意: xs_init()自体からeval_pv()を呼ぶとまだパーサの準備が整っておらず
 * クラッシュする(nested yyparseがxs_init実行タイミングでは早すぎる)。
 * そこでmain.cがargvに"-MRomperl::Boot"を挿入し、通常のuse文と同じ
 * (BEGIN時の)安全なタイミングでこのimport()経由でeval_pvを呼ぶように
 * している。%INCにあらかじめ偽のエントリを入れておき、実体の無い
 * "Romperl/Boot.pm" をrequireがディスクへ探しに行かないようにする。
 *
 * xsubppは使わず、XS()マクロで手書きする(通常のXSモジュールと同じ書き方)。
 */
#include "EXTERN.h"
#include "perl.h"
#include "XSUB.h"

#include "romfs.h"

XS(XS_Romperl_romfs_read); /* prototype to silence -Wmissing-prototypes */

XS(XS_Romperl_romfs_read)
{
    dXSARGS;
    STRLEN plen;
    const char *path;
    const void *data;
    unsigned long size;
    SV *result;

    if (items != 1)
        Perl_croak(aTHX_ "Usage: Romperl::romfs_read(path)");

    path = SvPV(ST(0), plen);

    data = romfs_data(path, &size);
    if (!data) {
        ST(0) = &PL_sv_undef;
        XSRETURN(1);
    }

    /*
     * ゼロコピー: SVのPVバッファをmallocせず、ROMFSイメージ上のバイト列
     * そのものを直接指す。SvLEN(result)=0は「このSVはPVバッファを所有して
     * いない」という意味で、Perl内部の共有ハッシュキー文字列等と同じ
     * 表現(sv.cのPerl_sv_clearはSvLENが0ならSafefreeしないので、
     * このSVが解放されてもROMFS側のメモリを誤って自由に触ることはない)。
     * このSVが後で sv_chop/sv_grow 等で書き換えを必要とされた場合は
     * Perl側が自動的にオウンドコピーへ昇格させるので安全性も保たれる
     * (例えば require の filter_cache 経路は1行読むごとに sv_chop するため、
     * 最初の消費で結局コピーされる。だがそれ以外の「文字列として読むだけ」
     * の用途では最後までmalloc/memcpyが一切発生しない)。
     */
    result = newSV(0);
    sv_upgrade(result, SVt_PV);
    SvPV_set(result, (char *)data);
    SvCUR_set(result, (STRLEN)size);
    SvLEN_set(result, 0);
    SvPOK_on(result);

    ST(0) = sv_2mortal(result);
    XSRETURN(1);
}

/*
 * @INC にコードリファレンスを積むだけのブートストラップ。require/use が
 * 通常の @INC 探索で見つからなかった場合にこのsubが呼ばれ、
 * "lib/<filename>" をROMFSから読めれば、そのスカラーへのリファレンスを
 * 返す(pp_requireがfilter_cacheとしてそのまま読み込む。PerlIO不要)。
 * 見つからなければ何も返さず、次の @INC エントリの探索に委ねる。
 */
static const char romperl_bootstrap_pl[] =
    "push @INC, sub {\n"
    "    my (undef, $filename) = @_;\n"
    "    my $src = Romperl::romfs_read(\"lib/$filename\");\n"
    "    return unless defined $src;\n"
    "    return \\$src;\n"
    "};\n";

XS(XS_Romperl_Boot_import); /* prototype to silence -Wmissing-prototypes */

XS(XS_Romperl_Boot_import)
{
    dXSARGS;
    PERL_UNUSED_VAR(items);
    eval_pv(romperl_bootstrap_pl, TRUE);
    XSRETURN_EMPTY;
}

void
romperl_xs_init(pTHX)
{
    newXS("Romperl::romfs_read", XS_Romperl_romfs_read, __FILE__);
    newXS("Romperl::Boot::import", XS_Romperl_Boot_import, __FILE__);
    /* "-MRomperl::Boot" が require しに行かないよう、実体の無い
     * Romperl/Boot.pm を読み込み済み扱いにしておく。 */
    hv_store(get_hv("INC", GV_ADD), "Romperl/Boot.pm", 15, newSViv(1), 0);
}
