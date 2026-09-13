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
    struct romfs_stat st;
    int fd;
    SV *result;

    if (items != 1)
        Perl_croak(aTHX_ "Usage: Romperl::romfs_read(path)");

    path = SvPV(ST(0), plen);

    if (romfs_stat(path, &st) != 0) {
        ST(0) = &PL_sv_undef;
        XSRETURN(1);
    }

    fd = romfs_open(path);
    if (fd < 0) {
        ST(0) = &PL_sv_undef;
        XSRETURN(1);
    }

    result = newSV(st.size);
    SvPOK_on(result);
    if (st.size) {
        char *buf = SvPVX(result);
        long n = romfs_read(fd, buf, st.size);
        SvCUR_set(result, n > 0 ? (STRLEN)n : 0);
    } else {
        SvCUR_set(result, 0);
    }
    *SvEND(result) = '\0';
    romfs_close(fd);

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
