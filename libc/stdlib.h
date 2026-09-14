/*
 * picoperl向けlibcシム: <stdlib.h>
 *
 * getenv()/putenv()をpicoperl自前の環境変数ストア(libc/env.c)
 * 経由のpicoperl_getenv()/picoperl_putenv()に、qsort()を内製の
 * 挿入ソート(libc/sort.c)経由のpicoperl_qsort()に、
 * rand()/srand()を内製のPRNG(libc/rand.c)経由の
 * picoperl_rand()/picoperl_srand()にリダイレクトする。
 *
 * setenv()/unsetenv()は対象外(このビルドは`d_unsetenv='undef'`で
 * Perl側のmy_setenv()もunsetenv()を呼ばない。putenv()だけで足りる)。
 *
 * libc依存を削るのはromperl側だけでよく、plain picoperlはNV=floatの
 * 最小実装のままにする方針のため(TODO.md「Phase 5」参照)、
 * stat/unlink/fopen系と同じ「弱いデフォルト実装(project rootの
 * env_shim.c/sort_shim.c/rand_shim.c、本物のlibc関数へのパススルー)+
 * romperl側の強い実装(リンク時にpp_requireのromperl_find_for_compileと
 * 同じ弱い/強いシンボルの仕組みで上書き)」という型を使う。plain
 * picoperlはenv_shim.c/sort_shim.c/rand_shim.cの弱いデフォルトが
 * そのまま使われるため無変更。
 */
#ifndef PICOPERL_LIBC_STDLIB_H
#define PICOPERL_LIBC_STDLIB_H

#include_next <stdlib.h>

extern const char *picoperl_getenv(const char *name);
extern int picoperl_putenv(char *string);
extern void picoperl_qsort(void *base, size_t nmemb, size_t size,
                            int (*compar)(const void *, const void *));
extern void picoperl_srand(unsigned int seed);
extern int  picoperl_rand(void);

#undef  getenv
#define getenv(name)      ((char *)picoperl_getenv((name)))
#undef  putenv
#define putenv(string)    picoperl_putenv((string))
#undef  qsort
#define qsort(base, nmemb, size, compar) picoperl_qsort((base), (nmemb), (size), (compar))
#undef  srand
#define srand(seed)       picoperl_srand((seed))
#undef  rand
#define rand(...)         picoperl_rand()

#endif /* PICOPERL_LIBC_STDLIB_H */
