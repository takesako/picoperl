/*
 * sort_shim.c - plain picoperl向けの既定実装。
 *
 * libc-pico2/stdlib.h は qsort を picoperl_qsort() にリダイレクトする
 * 関数マクロを定義している。この既定実装は「常に本物のlibc qsort(3)へ
 * そのまま委譲するだけ」のパススルーで、plain picoperlの挙動は今までと
 * 一切変わらない。
 *
 * __attribute__((weak))にしているのは、romperl(../sort/sort.c)が
 * 同名の強いシンボルでリンク時に上書きし、libcに依存しない自前の
 * 挿入ソートに差し替えられるようにするため。romperlはpicoperl-5.12.5の
 * .oをコピーせず参照するだけなので(このファイルも例外ではない)、
 * コンパイル時の#ifdefでpicoperl/romperlを出し分けることができず、
 * pp_requireのromperl_find_for_compileと同じ「弱いデフォルト+強い
 * 上書き」のリンク時解決に頼っている(env_shim.c/posix_shim.c/
 * stdio_shim.cと同じ仕組み)。
 *
 * 呼び出し先を`(qsort)(...)`のように余分な括弧で囲っているのは、
 * libc-pico2/stdlib.hが同名を関数マクロに置き換えているため、素の
 * `qsort(...)`と書くと自分自身(picoperl_qsort)を再帰呼び出しして
 * しまうのを防ぐため。
 */
#include <stdlib.h>

__attribute__((weak)) void
picoperl_qsort(void *base, size_t nmemb, size_t size,
               int (*compar)(const void *, const void *))
{
    (qsort)(base, nmemb, size, compar);
}
