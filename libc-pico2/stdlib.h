/*
 * picoperl向けlibcシム: <stdlib.h>
 *
 * getenv()/putenv()をpicoperl自前の環境変数ストア(../env/env.c)経由の
 * picoperl_getenv()/picoperl_putenv()にリダイレクトする。
 *
 * setenv()/unsetenv()は対象外(このビルドは`d_unsetenv='undef'`で
 * Perl側のmy_setenv()もunsetenv()を呼ばない。putenv()だけで足りる)。
 *
 * romfs/ramfs/vfs(ファイルI/O)とは異なり、picoperlとromperlで実装を
 * 出し分ける必要が無い(env.cはromfs/ramfsのような「ROMFS埋め込み
 * 対read-only」という区別を持たない、単なるlibc依存の置き換えのため)。
 * env.oはpicoperl-5.12.5自身のビルドに組み込まれ、romperlはその.oを
 * 参照するだけで自動的に同じ実装を使う。
 */
#ifndef PICOPERL_LIBC_PICO2_STDLIB_H
#define PICOPERL_LIBC_PICO2_STDLIB_H

#include_next <stdlib.h>

extern const char *picoperl_getenv(const char *name);
extern int picoperl_putenv(char *string);

#undef  getenv
#define getenv(name)      ((char *)picoperl_getenv((name)))
#undef  putenv
#define putenv(string)    picoperl_putenv((string))

#endif /* PICOPERL_LIBC_PICO2_STDLIB_H */
