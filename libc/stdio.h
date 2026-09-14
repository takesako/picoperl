/*
 * picoperl向けlibcシム: <stdio.h>
 *
 * fopen系(fopen/fclose/fread/fwrite/fseek/ftell/feof/ferror/clearerr/
 * fflush/fgetc/fputs/fileno/fprintf)をvfs(romfs+ramfs)経由の
 * picoperl_*()にリダイレクトする。
 *
 * このビルド(useperlio=undef)では、Perlの通常のopen()は
 * perlsdio.hの`#define PerlIO_open PerlSIO_fopen`によりfopen()に
 * 直結しており、POSIXの生fd(open/close/read/write/lseek)やfdopen()を
 * 一切経由しない(sysopenだけが例外だが、対象外として割り切っている。
 * 詳細はvfs/vfs_stdio.h・TODO.md「Phase 5」参照)。そのためfopen系だけを
 * リダイレクトすれば、通常のopen/print/読み込み/close は動く。
 *
 * fdopen/tmpfileは意図的にリダイレクトしていない(fdopenは本物のfdを
 * 前提とする関数で、vfs経由のfdという概念自体が無い。tmpfileは
 * 未対応のまま今まで通り本物の一時ファイルを使う)。
 */
#ifndef PICOPERL_LIBC_STDIO_H
#define PICOPERL_LIBC_STDIO_H

#include_next <stdio.h>

extern FILE  *picoperl_fopen(const char *path, const char *mode);
extern int    picoperl_fclose(FILE *fp);
extern size_t picoperl_fread(void *ptr, size_t size, size_t nmemb, FILE *fp);
extern size_t picoperl_fwrite(const void *ptr, size_t size, size_t nmemb, FILE *fp);
extern int    picoperl_fseek(FILE *fp, long offset, int whence);
extern long   picoperl_ftell(FILE *fp);
extern int    picoperl_feof(FILE *fp);
extern int    picoperl_ferror(FILE *fp);
extern void   picoperl_clearerr(FILE *fp);
extern int    picoperl_fflush(FILE *fp);
extern int    picoperl_fgetc(FILE *fp);
extern int    picoperl_fputs(const char *s, FILE *fp);
extern int    picoperl_fileno(FILE *fp);
extern int    picoperl_fprintf(FILE *fp, const char *format, ...);

#undef  fopen
#define fopen(path, mode)              picoperl_fopen((path), (mode))
#undef  fclose
#define fclose(fp)                     picoperl_fclose((fp))
#undef  fread
#define fread(ptr, size, nmemb, fp)    picoperl_fread((ptr), (size), (nmemb), (fp))
#undef  fwrite
#define fwrite(ptr, size, nmemb, fp)   picoperl_fwrite((ptr), (size), (nmemb), (fp))
#undef  fseek
#define fseek(fp, off, whence)         picoperl_fseek((fp), (off), (whence))
#undef  ftell
#define ftell(fp)                      picoperl_ftell((fp))
#undef  feof
#define feof(fp)                       picoperl_feof((fp))
#undef  ferror
#define ferror(fp)                     picoperl_ferror((fp))
#undef  clearerr
#define clearerr(fp)                   picoperl_clearerr((fp))
#undef  fflush
#define fflush(fp)                     picoperl_fflush((fp))
#undef  fgetc
#define fgetc(fp)                      picoperl_fgetc((fp))
#undef  fputs
#define fputs(s, fp)                   picoperl_fputs((s), (fp))
#undef  fileno
#define fileno(fp)                     picoperl_fileno((fp))
#undef  fprintf
#define fprintf(...)                   picoperl_fprintf(__VA_ARGS__)

#endif /* PICOPERL_LIBC_STDIO_H */
