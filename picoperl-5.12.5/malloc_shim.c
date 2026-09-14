/*
 * malloc_shim.c - plain picoperl向けの既定実装。
 *
 * libc/stdlib.h は malloc/calloc/realloc/free を picoperl_malloc()等に
 * リダイレクトする関数マクロを定義している。この既定実装は「常に本物の
 * libc malloc(3)/calloc(3)/realloc(3)/free(3)へそのまま委譲するだけ」の
 * パススルーで、plain picoperlの挙動は今までと一切変わらない。
 *
 * __attribute__((weak))にしているのは、romperl(../libc/malloc.c)が
 * 同名の強いシンボルでリンク時に上書きし、libcに依存しない自前の
 * 固定ヒープアロケータに差し替えられるようにするため。romperlは
 * picoperl-5.12.5の.oをコピーせず参照するだけなので(このファイルも
 * 例外ではない)、コンパイル時の#ifdefでpicoperl/romperlを出し分ける
 * ことができず、pp_requireのromperl_find_for_compileと同じ「弱い
 * デフォルト+強い上書き」のリンク時解決に頼っている(env_shim.c等と
 * 同じ仕組み)。
 *
 * 呼び出し先を`(malloc)(...)`のように余分な括弧で囲っているのは、
 * libc/stdlib.hが同名を関数マクロに置き換えているため、素の
 * `malloc(...)`と書くと自分自身を再帰呼び出ししてしまうのを防ぐため。
 */
#include <stdlib.h>

__attribute__((weak)) void *
picoperl_malloc(size_t size)
{
    return (malloc)(size);
}

__attribute__((weak)) void *
picoperl_calloc(size_t nmemb, size_t size)
{
    return (calloc)(nmemb, size);
}

__attribute__((weak)) void *
picoperl_realloc(void *ptr, size_t size)
{
    return (realloc)(ptr, size);
}

__attribute__((weak)) void
picoperl_free(void *ptr)
{
    (free)(ptr);
}
