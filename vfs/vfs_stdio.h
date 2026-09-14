/*
 * vfs_stdio.h - vfs.h(romfs+ramfsディスパッチ)をlibcのfopen系APIとして
 * 使えるようにする薄いラッパー。libc-pico2/stdio.hが差し替える
 * fopen/fclose/fread/fwrite/fseek/ftell/feof/ferror/clearerr/fflush/
 * fgetc/fputs/fileno/fprintfの実体をここに置く。
 *
 * 経緯(TODO.md「Phase 5」参照): 当初POSIXの生fd(open/close/read/write/
 * lseek)をvfsにリダイレクトしようとしたが、このビルド(useperlio=undef)
 * ではsysopenだけがopen()+本物のfdopen()という経路を通り、fdopen()に
 * 偽fdを渡すと失敗することが判明した。一方、Perlの通常のopen()は
 * perlsdio.hの`#define PerlIO_open PerlSIO_fopen`によりfopen()に
 * 直結しており、open()/fdopen()を一切経由しない。「自作libcに置き換えた
 * 際にPOSIXの生fd系(sysopen相当)が呼ばれないなら、fopen系だけ対応すれば
 * 良い」という方針に基づき、ここではfopen系だけをvfsに繋ぐ。
 *
 * FILE*の扱い: ここでのfopen()はglibc本物のFILE*ではなく、内部で
 * malloc()した独自の構造体(vfs_FILEをラップし、magic numberで
 * 目印を付けたもの)へのポインタをFILE*として返す(呼び出し側は
 * 常にこのヘッダ経由の関数だけを介して触るオペークハンドルとして
 * 扱うので、実際の構造体レイアウトの違いは問題にならない)。
 * fread/fwrite/fclose等は、渡されたFILE*がこのmagicを持つか調べ、
 * 持たなければ「自分が作ったのではない本物のFILE*」(stdin/stdout/
 * stderr等)とみなして本物のシステムのfread/fwrite/fclose等に
 * そのまま委譲する。この判定はポインタ先頭のマジックナンバー1つで
 * 行っており厳密な安全性の証明は無いが、本物のglibc FILE構造体の
 * 同じオフセットに偶然一致する可能性は無視できるほど低い
 * (シンプルさ優先の方針に基づく割り切り)。
 */
#ifndef VFS_STDIO_H
#define VFS_STDIO_H

#include <stdio.h>
#include <stdarg.h>

FILE  *picoperl_fopen(const char *path, const char *mode);
int    picoperl_fclose(FILE *fp);
size_t picoperl_fread(void *ptr, size_t size, size_t nmemb, FILE *fp);
size_t picoperl_fwrite(const void *ptr, size_t size, size_t nmemb, FILE *fp);
int    picoperl_fseek(FILE *fp, long offset, int whence);
long   picoperl_ftell(FILE *fp);
int    picoperl_feof(FILE *fp);
int    picoperl_ferror(FILE *fp);
void   picoperl_clearerr(FILE *fp);
int    picoperl_fflush(FILE *fp);
int    picoperl_fgetc(FILE *fp);
int    picoperl_fputs(const char *s, FILE *fp);
int    picoperl_fileno(FILE *fp);
int    picoperl_fprintf(FILE *fp, const char *format, ...);

#endif /* VFS_STDIO_H */
