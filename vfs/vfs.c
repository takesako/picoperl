/*
 * vfs.c - vfs.h の実装。romfs/ramfsそのものの中身には手を入れず、
 * どちらを呼ぶかを振り分けるだけの薄いラッパー。
 */
#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include "vfs.h"
#include "ramfs.h"
#include "romfs.h"

struct vfs_FILE {
    int is_rom;      /* 1: romfsのfdをラップ, 0: ramfs_FILE*をラップ */
    int eof;         /* is_romの場合のみ使う (ramfs側はramfs_feofに委譲) */
    ramfs_FILE *ram;
    int romfd;
};

static int
mode_wants_write(const char *mode)
{
    if (!mode || !mode[0])
        return 0;
    return mode[0] == 'w' || mode[0] == 'a' || strchr(mode, '+') != NULL;
}

/*
 * romfs/ramfsはどちらも完全一致のフラットな名前空間しか持たないため、
 * "./foo.pm"と"foo.pm"を別名として扱ってしまう。Perlの@INC探索は
 * カレントディレクトリのエントリ"."に対して"./foo.pm"のような
 * パスを作ってopen()を試みるため、これを剥がしておかないと
 * requireで見つかるはずのramfs/romfs上のファイルが見つからない
 * (`./`が付くだけで別ファイル扱いになる)。"./"が繰り返されている
 * 場合(まず無いはずだが)も一応剥がす。
 */
static const char *
normalize(const char *name)
{
    while (name[0] == '.' && name[1] == '/')
        name += 2;
    return name;
}

void
vfs_init(void)
{
    ramfs_init();
}

vfs_FILE *
vfs_fopen(const char *name, const char *mode)
{
    vfs_FILE *fp;

    if (!name || !mode)
        return NULL;
    name = normalize(name);

    fp = (vfs_FILE *)malloc(sizeof *fp);
    if (!fp)
        return NULL;
    fp->eof = 0;

    if (mode_wants_write(mode)) {
        /* 書き込みを伴うopenは常にramfsだけを対象にする。romfsは
         * read-onlyという前提を絶対に崩さないため、名前がromfsと
         * 衝突していてもromfs側には一切触れない。
         */
        fp->ram = ramfs_fopen(name, mode);
        if (!fp->ram) {
            free(fp);
            return NULL;
        }
        fp->is_rom = 0;
        return fp;
    }

    /* 読み取り専用: ramfsを優先し、無ければromfsにフォールバックする。 */
    fp->ram = ramfs_fopen(name, "r");
    if (fp->ram) {
        fp->is_rom = 0;
        return fp;
    }

    fp->romfd = romfs_open(name);
    if (fp->romfd >= 0) {
        fp->is_rom = 1;
        return fp;
    }

    free(fp);
    return NULL;
}

unsigned long
vfs_fread(void *ptr, unsigned long size, unsigned long nmemb, vfs_FILE *fp)
{
    long n;

    if (!fp || size == 0 || nmemb == 0)
        return 0;

    if (fp->is_rom) {
        n = romfs_read(fp->romfd, ptr, size * nmemb);
        if (n < 0)
            n = 0;
        if ((unsigned long)n < size * nmemb)
            fp->eof = 1;
        return (unsigned long)n / size;
    }

    return ramfs_fread(ptr, size, nmemb, fp->ram);
}

unsigned long
vfs_fwrite(const void *ptr, unsigned long size, unsigned long nmemb, vfs_FILE *fp)
{
    if (!fp || fp->is_rom)
        return 0; /* romfsは書き込み不可 */
    return ramfs_fwrite(ptr, size, nmemb, fp->ram);
}

int
vfs_fseek(vfs_FILE *fp, long offset, int whence)
{
    if (!fp)
        return -1;

    if (fp->is_rom) {
        /* VFS_SEEK_*とROMFS_SEEK_*は同じ値(0/1/2)なのでそのまま渡せる */
        if (romfs_lseek(fp->romfd, offset, whence) < 0)
            return -1;
        fp->eof = 0;
        return 0;
    }

    return ramfs_fseek(fp->ram, offset, whence);
}

long
vfs_ftell(vfs_FILE *fp)
{
    if (!fp)
        return -1;
    if (fp->is_rom)
        return romfs_lseek(fp->romfd, 0, ROMFS_SEEK_CUR);
    return ramfs_ftell(fp->ram);
}

int
vfs_feof(vfs_FILE *fp)
{
    if (!fp)
        return 0;
    if (fp->is_rom)
        return fp->eof;
    return ramfs_feof(fp->ram);
}

int
vfs_fclose(vfs_FILE *fp)
{
    int rc;

    if (!fp)
        return -1;

    rc = fp->is_rom ? romfs_close(fp->romfd) : ramfs_fclose(fp->ram);
    free(fp);
    return rc;
}

int
vfs_remove(const char *name)
{
    if (!name)
        return -1;
    name = normalize(name);
    /* romfs上のファイルは削除できない(read-only前提を崩さない)。 */
    return ramfs_remove(name);
}

int
vfs_stat(const char *name, struct vfs_stat *st)
{
    struct ramfs_stat rst;
    struct romfs_stat rmst;

    if (!name)
        return -1;
    name = normalize(name);

    if (ramfs_stat(name, &rst) == 0) {
        if (st)
            st->size = rst.size;
        return 0;
    }
    if (romfs_stat(name, &rmst) == 0) {
        if (st)
            st->size = rmst.size;
        return 0;
    }
    return -1;
}
