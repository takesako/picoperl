/*
 * vfs_stdio.c - vfs_stdio.h の実装。romperl専用の強い実装で、
 * ../stdio_shim.c の弱いデフォルト(本物のシステムコールへの
 * パススルー)をリンク時に上書きする(pp_requireのromperl_find_for_compile
 * と同じ「弱いデフォルト+強い上書き」の仕組み)。
 *
 * 呼び出し先を`(fread)(...)`のように余分な括弧で囲っているのは、
 * libc/stdio.hが同名を関数マクロに置き換えているため、素の
 * `fread(...)`と書くと自分自身を再帰呼び出ししてしまうのを防ぐため。
 */
#include <string.h>
#include <stdlib.h>
#include "vfs.h"
#include "vfs_stdio.h"

#define VFS_STDIO_MAGIC 0x504F5346u /* 'PSOF' (Pico-perl Stdio Open File) */

struct vfs_stdio_file {
    unsigned magic;
    vfs_FILE *vf;
    int err;
};

static struct vfs_stdio_file *
as_ours(FILE *fp)
{
    struct vfs_stdio_file *f = (struct vfs_stdio_file *)(void *)fp;

    if (f && f->magic == VFS_STDIO_MAGIC)
        return f;
    return NULL;
}

FILE *
picoperl_fopen(const char *path, const char *mode)
{
    vfs_FILE *vf;
    struct vfs_stdio_file *f;

    vf = vfs_fopen(path, mode);
    if (!vf) {
        /* 読み取り専用("r"、"+"を含まない)でvfs(ramfs+romfs)に無ければ、
         * 開発ホスト上の実ファイルシステムにフォールバックする。
         * これはromperl自身がPerlスクリプトファイルを引数で受け取って
         * 開く経路(perl.cのスクリプト読み込みも同じfopen()を通る)を
         * 壊さないために必要(このフォールバックが無いと
         * `./romperl foo.pl`が常に失敗する)。書き込みを伴うモードは
         * vfs_fopenがほぼ確実に成功するため通常ここには来ないが、
         * 来たとしても実ファイルシステムには書かせない(vfsの
         * 書き込みは常にvfs専用、という方針を崩さない)。
         * 実機(RP2350、実ファイルシステム自体が存在しない)では
         * このフォールバックは単に失敗するだけで無害。
         */
        if (mode[0] == 'r' && mode[1] != '+')
            return (fopen)(path, mode);
        return NULL;
    }

    f = (struct vfs_stdio_file *)malloc(sizeof *f);
    if (!f) {
        vfs_fclose(vf);
        return NULL;
    }
    f->magic = VFS_STDIO_MAGIC;
    f->vf = vf;
    f->err = 0;
    return (FILE *)(void *)f;
}

int
picoperl_fclose(FILE *fp)
{
    struct vfs_stdio_file *f = as_ours(fp);
    int rc;

    if (!f)
        return (fclose)(fp);
    rc = vfs_fclose(f->vf);
    free(f);
    return rc;
}

size_t
picoperl_fread(void *ptr, size_t size, size_t nmemb, FILE *fp)
{
    struct vfs_stdio_file *f = as_ours(fp);

    if (!f)
        return (fread)(ptr, size, nmemb, fp);
    return (size_t)vfs_fread(ptr, (unsigned long)size, (unsigned long)nmemb, f->vf);
}

size_t
picoperl_fwrite(const void *ptr, size_t size, size_t nmemb, FILE *fp)
{
    struct vfs_stdio_file *f = as_ours(fp);
    size_t n;

    if (!f)
        return (fwrite)(ptr, size, nmemb, fp);
    n = (size_t)vfs_fwrite(ptr, (unsigned long)size, (unsigned long)nmemb, f->vf);
    if (n < nmemb)
        f->err = 1;
    return n;
}

int
picoperl_fseek(FILE *fp, long offset, int whence)
{
    struct vfs_stdio_file *f = as_ours(fp);

    if (!f)
        return (fseek)(fp, offset, whence);
    return vfs_fseek(f->vf, offset, whence);
}

long
picoperl_ftell(FILE *fp)
{
    struct vfs_stdio_file *f = as_ours(fp);

    if (!f)
        return (ftell)(fp);
    return vfs_ftell(f->vf);
}

int
picoperl_feof(FILE *fp)
{
    struct vfs_stdio_file *f = as_ours(fp);

    if (!f)
        return (feof)(fp);
    return vfs_feof(f->vf);
}

int
picoperl_ferror(FILE *fp)
{
    struct vfs_stdio_file *f = as_ours(fp);

    if (!f)
        return (ferror)(fp);
    return f->err;
}

void
picoperl_clearerr(FILE *fp)
{
    struct vfs_stdio_file *f = as_ours(fp);

    if (!f) {
        (clearerr)(fp);
        return;
    }
    f->err = 0;
}

int
picoperl_fflush(FILE *fp)
{
    /* vfs(ramfs/romfs)は無バッファなので何もすることが無い。 */
    if (!as_ours(fp))
        return (fflush)(fp);
    return 0;
}

int
picoperl_fgetc(FILE *fp)
{
    struct vfs_stdio_file *f = as_ours(fp);
    unsigned char c;

    if (!f)
        return (fgetc)(fp);
    if (vfs_fread(&c, 1, 1, f->vf) != 1)
        return EOF;
    return (int)c;
}

int
picoperl_fputs(const char *s, FILE *fp)
{
    struct vfs_stdio_file *f = as_ours(fp);
    size_t len;

    if (!f)
        return (fputs)(s, fp);
    len = strlen(s);
    if (vfs_fwrite(s, 1, len, f->vf) != len) {
        f->err = 1;
        return EOF;
    }
    return 0;
}

int
picoperl_fileno(FILE *fp)
{
    /* vfsバックエンドには本物のfdが存在しないため、-1(エラー)を返す。
     * 本物のFILE*(stdin/stdout/stderr等)はそのまま委譲する。 */
    if (!as_ours(fp))
        return (fileno)(fp);
    return -1;
}

int
picoperl_fprintf(FILE *fp, const char *format, ...)
{
    struct vfs_stdio_file *f = as_ours(fp);
    va_list ap;
    char buf[1024];
    int n;

    if (!f) {
        int rc;
        va_start(ap, format);
        rc = (vfprintf)(fp, format, ap);
        va_end(ap);
        return rc;
    }

    va_start(ap, format);
    n = vsnprintf(buf, sizeof buf, format, ap);
    va_end(ap);
    if (n < 0)
        return n;

    if ((size_t)n < sizeof buf) {
        vfs_fwrite(buf, 1, (unsigned long)n, f->vf);
    } else {
        char *big = (char *)malloc((size_t)n + 1);
        if (!big)
            return -1;
        va_start(ap, format);
        vsnprintf(big, (size_t)n + 1, format, ap);
        va_end(ap);
        vfs_fwrite(big, 1, (unsigned long)n, f->vf);
        free(big);
    }
    return n;
}
