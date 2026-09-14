/*
 * env_shim.c - plain picoperl向けの既定実装。
 *
 * libc/stdlib.h は getenv/putenv を picoperl_getenv()/
 * picoperl_putenv() にリダイレクトする関数マクロを定義している。
 * この既定実装は「常に本物のlibc関数へそのまま委譲するだけ」の
 * パススルーで、plain picoperlの挙動は今までと一切変わらない。
 *
 * __attribute__((weak))にしているのは、romperl(../libc/env.c)が同名の
 * 強いシンボルでリンク時に上書きし、libcに依存しない自前の環境変数
 * ストアに差し替えられるようにするため。romperlはpicoperl-5.12.5の
 * .oをコピーせず参照するだけなので(このファイルも例外ではない)、
 * コンパイル時の#ifdefでpicoperl/romperlを出し分けることができず、
 * pp_requireのromperl_find_for_compileと同じ「弱いデフォルト+強い
 * 上書き」のリンク時解決に頼っている。
 *
 * romfs/ramfs/vfs(ファイルI/O)と同じ理由でこの仕組みが必要: libc依存を
 * 削るのはromperl側だけでよく、plain picoperlはNV=floatの最小実装の
 * ままにする、という方針のため(TODO.md「Phase 5」参照)。
 *
 * 呼び出し先を`(getenv)(...)`のように余分な括弧で囲っているのは、
 * libc/stdlib.hが同名を関数マクロに置き換えているため、素の
 * `getenv(...)`と書くと自分自身(picoperl_getenv)を再帰呼び出しして
 * しまうのを防ぐため(関数マクロは「識別子の直後に'('」が展開条件のため、
 * `(getenv)`のように直後が'('でなければ展開されず本物のシンボルを指す)。
 */
#include <stdlib.h>

__attribute__((weak)) const char *
picoperl_getenv(const char *name)
{
    return (getenv)(name);
}

__attribute__((weak)) int
picoperl_putenv(char *string)
{
    return (putenv)(string);
}
