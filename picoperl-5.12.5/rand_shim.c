/*
 * rand_shim.c - plain picoperl向けの既定実装。
 *
 * libc-pico2/stdlib.h は rand/srand を picoperl_rand()/picoperl_srand()
 * にリダイレクトする関数マクロを定義している。この既定実装は「常に
 * 本物のlibc rand(3)/srand(3)へそのまま委譲するだけ」のパススルーで、
 * plain picoperlの挙動は今までと一切変わらない。
 *
 * __attribute__((weak))にしているのは、romperl(../rand/rand.c)が
 * 同名の強いシンボルでリンク時に上書きし、libcに依存しない自前の
 * PRNG(線形合同法)に差し替えられるようにするため。romperlは
 * picoperl-5.12.5の.oをコピーせず参照するだけなので(このファイルも
 * 例外ではない)、コンパイル時の#ifdefでpicoperl/romperlを出し分ける
 * ことができず、pp_requireのromperl_find_for_compileと同じ「弱い
 * デフォルト+強い上書き」のリンク時解決に頼っている(env_shim.c/
 * posix_shim.c/stdio_shim.c/sort_shim.cと同じ仕組み)。
 *
 * 呼び出し先を`(rand)(...)`のように余分な括弧で囲っているのは、
 * libc-pico2/stdlib.hが同名を関数マクロに置き換えているため、素の
 * `rand(...)`と書くと自分自身(picoperl_rand)を再帰呼び出しして
 * しまうのを防ぐため。
 */
#include <stdlib.h>

__attribute__((weak)) void
picoperl_srand(unsigned int seed)
{
    (srand)(seed);
}

__attribute__((weak)) int
picoperl_rand(void)
{
    return (rand)();
}
