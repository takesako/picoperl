/*
 * picoperl向けlibcシム: <stdlib.h>
 *
 * getenv()/putenv()をpicoperl自前の環境変数ストア(romperl/env/env.c)
 * 経由のpicoperl_getenv()/picoperl_putenv()に、qsort()を内製の
 * 挿入ソート(romperl/sort/sort.c)経由のpicoperl_qsort()にリダイレクト
 * する。
 *
 * setenv()/unsetenv()は対象外(このビルドは`d_unsetenv='undef'`で
 * Perl側のmy_setenv()もunsetenv()を呼ばない。putenv()だけで足りる)。
 *
 * libc依存を削るのはromperl側だけでよく、plain picoperlはNV=floatの
 * 最小実装のままにする方針のため(TODO.md「Phase 5」参照)、
 * stat/unlink/fopen系と同じ「弱いデフォルト実装(project rootの
 * env_shim.c/sort_shim.c、本物のlibc関数へのパススルー)+ romperl側の
 * 強い実装(リンク時にpp_requireのromperl_find_for_compileと同じ
 * 弱い/強いシンボルの仕組みで上書き)」という型を使う。plain picoperlは
 * env_shim.c/sort_shim.cの弱いデフォルトがそのまま使われるため無変更。
 */
#ifndef PICOPERL_LIBC_PICO2_STDLIB_H
#define PICOPERL_LIBC_PICO2_STDLIB_H

#include_next <stdlib.h>

extern const char *picoperl_getenv(const char *name);
extern int picoperl_putenv(char *string);
extern void picoperl_qsort(void *base, size_t nmemb, size_t size,
                            int (*compar)(const void *, const void *));

#undef  getenv
#define getenv(name)      ((char *)picoperl_getenv((name)))
#undef  putenv
#define putenv(string)    picoperl_putenv((string))
#undef  qsort
#define qsort(base, nmemb, size, compar) picoperl_qsort((base), (nmemb), (size), (compar))

#endif /* PICOPERL_LIBC_PICO2_STDLIB_H */
