/*
 * picoperl向けlibcシム: <sys/stat.h>
 *
 * stat()(パス名ベース)をvfs(romfs+ramfs)経由のpicoperl_stat()に
 * リダイレクトする。struct stat自体は本物のヘッダ(#include_nextで読み込む)
 * の定義をそのまま使う(Stat_tはunixish.hで`#define Stat_t struct stat`と
 * 定義されている)。
 *
 * fstat()(fdベース)は意図的にリダイレクトしていない。vfs経由のfdを
 * 作る手段(open()のリダイレクト)がまだ無い(TODO.md「Phase 5」参照:
 * fdopen/fopenを合わせて対応するまで保留)ため、今リダイレクトしても
 * 常に本物へパススルーするだけで意味が無い。
 */
#ifndef PICOPERL_LIBC_PICO2_SYS_STAT_H
#define PICOPERL_LIBC_PICO2_SYS_STAT_H

#include_next <sys/stat.h>

extern int picoperl_stat(const char *path, struct stat *st);

#undef  stat
#define stat(path, st) picoperl_stat((path), (st))

#endif /* PICOPERL_LIBC_PICO2_SYS_STAT_H */
