/*
 * ctype_shim.c - plain picoperl向けの既定実装。
 *
 * libc/ctype.h は is*系/to*系 を picoperl_is*()/picoperl_to*() にリダイレクト
 * する関数マクロを定義している。この既定実装は「常に本物のlibc
 * is*(3)/to*(3)へそのまま委譲するだけ」のパススルーで、plain picoperl
 * の挙動は今までと一切変わらない。
 *
 * __attribute__((weak))にしているのは、romperl(../libc/ctype.c)が
 * 同名の強いシンボルでリンク時に上書きし、libcのlocale依存テーブル
 * (__ctype_b_loc)に依存しない自前実装(ASCII範囲の単純な比較)に
 * 差し替えられるようにするため。romperlはpicoperl-5.12.5の.oを
 * コピーせず参照するだけなので(このファイルも例外ではない)、
 * コンパイル時の#ifdefでpicoperl/romperlを出し分けることができず、
 * pp_requireのromperl_find_for_compileと同じ「弱いデフォルト+強い
 * 上書き」のリンク時解決に頼っている(env_shim.c等と同じ仕組み)。
 *
 * 呼び出し先を`(isalpha)(...)`のように余分な括弧で囲っているのは、
 * libc/ctype.hが同名を関数マクロに置き換えているため、素の
 * `isalpha(...)`と書くと自分自身を再帰呼び出ししてしまうのを防ぐため。
 */
#include <ctype.h>

__attribute__((weak)) int picoperl_isalpha(int c)  { return (isalpha)(c); }
__attribute__((weak)) int picoperl_isdigit(int c)  { return (isdigit)(c); }
__attribute__((weak)) int picoperl_isspace(int c)  { return (isspace)(c); }
__attribute__((weak)) int picoperl_isupper(int c)  { return (isupper)(c); }
__attribute__((weak)) int picoperl_islower(int c)  { return (islower)(c); }
__attribute__((weak)) int picoperl_isalnum(int c)  { return (isalnum)(c); }
__attribute__((weak)) int picoperl_ispunct(int c)  { return (ispunct)(c); }
__attribute__((weak)) int picoperl_iscntrl(int c)  { return (iscntrl)(c); }
__attribute__((weak)) int picoperl_isprint(int c)  { return (isprint)(c); }
__attribute__((weak)) int picoperl_isgraph(int c)  { return (isgraph)(c); }
__attribute__((weak)) int picoperl_isxdigit(int c) { return (isxdigit)(c); }
__attribute__((weak)) int picoperl_isascii(int c)  { return (isascii)(c); }
__attribute__((weak)) int picoperl_isblank(int c)  { return (isblank)(c); }
__attribute__((weak)) int picoperl_toupper(int c)  { return (toupper)(c); }
__attribute__((weak)) int picoperl_tolower(int c)  { return (tolower)(c); }
