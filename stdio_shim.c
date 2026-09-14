/*
 * stdio_shim.c - plain picoperl向けの既定実装。
 *
 * libc-pico2/stdio.h は fopen/fclose/fread/fwrite/fseek/ftell/feof/
 * ferror/clearerr/fflush/fgetc/fputs/fileno/fprintf を picoperl_*() に
 * リダイレクトする関数マクロを定義している。この既定実装は「常に本物の
 * システムコールへそのまま委譲するだけ」のパススルーで、plain picoperl
 * の挙動は今までと一切変わらない。
 *
 * __attribute__((weak))にしているのは、romperl(../vfs/vfs_stdio.c)が
 * 同名の強いシンボルでリンク時に上書きし、ROMFS/RAMFS(vfs)経由の実装に
 * 差し替えられるようにするため。romperlはpicoperl-5.12.5の.oをコピー
 * せず参照するだけなので(このファイルも例外ではない)、コンパイル時の
 * #ifdefでpicoperl/romperlを出し分けることができず、pp_requireの
 * romperl_find_for_compileと同じ「弱いデフォルト+強い上書き」の
 * リンク時解決に頼っている。
 *
 * 呼び出し先を`(fopen)(...)`のように余分な括弧で囲っているのは、
 * libc-pico2/stdio.hが同名を関数マクロに置き換えているため、素の
 * `fopen(...)`と書くと自分自身(picoperl_fopen)を再帰呼び出しして
 * しまうのを防ぐため(関数マクロは「識別子の直後に'('」が展開条件のため、
 * `(fopen)`のように直後が'('でなければ展開されず本物のシンボルを指す)。
 */
#include <stdio.h>
#include <stdarg.h>

__attribute__((weak)) FILE *
picoperl_fopen(const char *path, const char *mode)
{
    return (fopen)(path, mode);
}

__attribute__((weak)) int
picoperl_fclose(FILE *fp)
{
    return (fclose)(fp);
}

__attribute__((weak)) size_t
picoperl_fread(void *ptr, size_t size, size_t nmemb, FILE *fp)
{
    return (fread)(ptr, size, nmemb, fp);
}

__attribute__((weak)) size_t
picoperl_fwrite(const void *ptr, size_t size, size_t nmemb, FILE *fp)
{
    return (fwrite)(ptr, size, nmemb, fp);
}

__attribute__((weak)) int
picoperl_fseek(FILE *fp, long offset, int whence)
{
    return (fseek)(fp, offset, whence);
}

__attribute__((weak)) long
picoperl_ftell(FILE *fp)
{
    return (ftell)(fp);
}

__attribute__((weak)) int
picoperl_feof(FILE *fp)
{
    return (feof)(fp);
}

__attribute__((weak)) int
picoperl_ferror(FILE *fp)
{
    return (ferror)(fp);
}

__attribute__((weak)) void
picoperl_clearerr(FILE *fp)
{
    (clearerr)(fp);
}

__attribute__((weak)) int
picoperl_fflush(FILE *fp)
{
    return (fflush)(fp);
}

__attribute__((weak)) int
picoperl_fgetc(FILE *fp)
{
    return (fgetc)(fp);
}

__attribute__((weak)) int
picoperl_fputs(const char *s, FILE *fp)
{
    return (fputs)(s, fp);
}

__attribute__((weak)) int
picoperl_fileno(FILE *fp)
{
    return (fileno)(fp);
}

__attribute__((weak)) int
picoperl_fprintf(FILE *fp, const char *format, ...)
{
    va_list ap;
    int rc;

    va_start(ap, format);
    rc = (vfprintf)(fp, format, ap);
    va_end(ap);
    return rc;
}
