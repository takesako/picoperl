/*
 * picoperl向けlibcシム: <ctype.h>
 *
 * is*系/to*系マクロ(内部でlocale依存の__ctype_b_loc(3)テーブルを参照する)
 * をpicoperl自前の実装(libc/ctype.c、"C"ロケール相当のASCII範囲比較)
 * 経由のpicoperl_is*()/picoperl_to*()にリダイレクトする。
 *
 * libc依存を削るのはromperl側だけでよく、plain picoperlはNV=floatの
 * 最小実装のままにする方針のため(TODO.md「Phase 5」参照)、他の
 * libcシムと同じ「弱いデフォルト実装(project rootのctype_shim.c、
 * 本物のlibc関数へのパススルー)+ romperl側の強い実装(リンク時に
 * pp_requireのromperl_find_for_compileと同じ弱い/強いシンボルの
 * 仕組みで上書き)」という型を使う。
 */
#ifndef PICOPERL_LIBC_CTYPE_H
#define PICOPERL_LIBC_CTYPE_H

#include_next <ctype.h>

extern int picoperl_isalpha(int c);
extern int picoperl_isdigit(int c);
extern int picoperl_isspace(int c);
extern int picoperl_isupper(int c);
extern int picoperl_islower(int c);
extern int picoperl_isalnum(int c);
extern int picoperl_ispunct(int c);
extern int picoperl_iscntrl(int c);
extern int picoperl_isprint(int c);
extern int picoperl_isgraph(int c);
extern int picoperl_isxdigit(int c);
extern int picoperl_isascii(int c);
extern int picoperl_isblank(int c);
extern int picoperl_toupper(int c);
extern int picoperl_tolower(int c);

#undef  isalpha
#define isalpha(c)  picoperl_isalpha((c))
#undef  isdigit
#define isdigit(c)  picoperl_isdigit((c))
#undef  isspace
#define isspace(c)  picoperl_isspace((c))
#undef  isupper
#define isupper(c)  picoperl_isupper((c))
#undef  islower
#define islower(c)  picoperl_islower((c))
#undef  isalnum
#define isalnum(c)  picoperl_isalnum((c))
#undef  ispunct
#define ispunct(c)  picoperl_ispunct((c))
#undef  iscntrl
#define iscntrl(c)  picoperl_iscntrl((c))
#undef  isprint
#define isprint(c)  picoperl_isprint((c))
#undef  isgraph
#define isgraph(c)  picoperl_isgraph((c))
#undef  isxdigit
#define isxdigit(c) picoperl_isxdigit((c))
#undef  isascii
#define isascii(c)  picoperl_isascii((c))
#undef  isblank
#define isblank(c)  picoperl_isblank((c))
#undef  toupper
#define toupper(c)  picoperl_toupper((c))
#undef  tolower
#define tolower(c)  picoperl_tolower((c))

#endif /* PICOPERL_LIBC_CTYPE_H */
