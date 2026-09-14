/*
 * vfs_posix.c - vfs_posix.h の実装。romperl専用の強い実装で、
 * ../posix_shim.c の弱いデフォルト(本物のシステムコールへの
 * パススルー)をリンク時に上書きする(pp_requireのromperl_find_for_compile
 * と同じ「弱いデフォルト+強い上書き」の仕組み)。
 *
 * stat/unlinkはパス名だけを扱う操作なので、vfs(romfs+ramfs)へ単独で
 * リダイレクトできる。open/close/read/write/lseek/fstatはここでは
 * 扱わない(vfs_posix.h参照: fdopen/fopenを合わせて対応するまで保留)。
 */
#include <string.h>
#include <sys/stat.h>
#include "vfs.h"
#include "vfs_posix.h"

static void
fill_stat(struct stat *st, unsigned long size)
{
    memset(st, 0, sizeof *st);
    st->st_mode = S_IFREG | 0644;
    st->st_nlink = 1;
    st->st_size = (off_t)size;
}

int
picoperl_stat(const char *path, struct stat *st)
{
    struct vfs_stat vst;

    if (vfs_stat(path, &vst) < 0)
        return -1;
    fill_stat(st, vst.size);
    return 0;
}

int
picoperl_unlink(const char *path)
{
    return vfs_remove(path) < 0 ? -1 : 0;
}
