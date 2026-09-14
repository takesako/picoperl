/*
 * vfs_posix.h - vfs.h(romfs+ramfsディスパッチ)をPOSIX風のstat/unlinkと
 * して使えるようにする薄いラッパー。libc-pico2の*.hシムが差し替える
 * stat/unlinkの実体をここに置く。
 *
 * stat/unlinkはパス名だけを扱いfd/FILE*を経由しないため、単独でvfs
 * バックエンドに切り替えられる。open/close/read/write/lseek/fstatは
 * 意図的に含めていない。TODO.md「Phase 5」参照: useperlio=undefの
 * このビルドでは、sysopen等がopen()で得たfdを直後に本物のfdopen()で
 * FILE*化するため、open()だけをvfsの偽fdにリダイレクトするとfdopen()が
 * EBADFで失敗する。fdopen/fopen以降のstdio全体(fread/fwrite/fclose/
 * fseek等)を合わせてvfs対応させる(次のTODO項目「stdioをPerlIO経由で
 * UARTに直結」)まで、fd経由の操作は全て保留する。
 *
 * この宣言はlibc-pico2側にも(cross-includeを避けるため)重複して
 * externで書かれている。シグネチャを変える場合は両方直すこと。
 */
#ifndef VFS_POSIX_H
#define VFS_POSIX_H

#include <sys/stat.h>

int picoperl_stat(const char *path, struct stat *st);
int picoperl_unlink(const char *path);

#endif /* VFS_POSIX_H */
