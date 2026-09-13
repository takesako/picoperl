/*
 * romfs.h で定義した最小 open/read/seek/close/stat API の実装。
 * イメージは常にメモリ上のバイト列(x86_64ではリンクされた.romfsセクション、
 * Cortex-M33ではFlashにmmapされた領域を想定)として扱うため、
 * ホスト側/ターゲット側で実装を分ける必要はない。
 */
#include <string.h>
#include "romfs.h"

struct romfs_fd {
    int used;
    const struct romfs_entry *entry;
    unsigned long pos;
};

static const unsigned char *romfs_base;
static const struct romfs_header *romfs_hdr;
static const struct romfs_entry *romfs_entries;
static struct romfs_fd romfs_fdtable[ROMFS_MAX_OPEN];

int
romfs_init(const unsigned char *image, unsigned long size)
{
    const struct romfs_header *hdr = (const struct romfs_header *)image;
    unsigned long need;
    int i;

    if (size < sizeof(struct romfs_header))
        return 0;
    if (memcmp(hdr->magic, ROMFS_MAGIC, sizeof(hdr->magic)) != 0)
        return 0;

    need = sizeof(struct romfs_header)
         + (unsigned long)hdr->file_count * sizeof(struct romfs_entry);
    if (need > size || hdr->total_size > size)
        return 0;

    romfs_base = image;
    romfs_hdr = hdr;
    romfs_entries = (const struct romfs_entry *)(image + sizeof(struct romfs_header));
    for (i = 0; i < ROMFS_MAX_OPEN; i++)
        romfs_fdtable[i].used = 0;
    return 1;
}

static const struct romfs_entry *
romfs_find(const char *path)
{
    uint32_t i;
    for (i = 0; i < romfs_hdr->file_count; i++) {
        const struct romfs_entry *e = &romfs_entries[i];
        if (strncmp(e->name, path, ROMFS_NAME_MAX) == 0)
            return e;
    }
    return (const struct romfs_entry *)0;
}

int
romfs_open(const char *path)
{
    const struct romfs_entry *e;
    int i;

    e = romfs_find(path);
    if (!e)
        return -1;

    for (i = 0; i < ROMFS_MAX_OPEN; i++) {
        if (!romfs_fdtable[i].used) {
            romfs_fdtable[i].used = 1;
            romfs_fdtable[i].entry = e;
            romfs_fdtable[i].pos = 0;
            return i;
        }
    }
    return -1;
}

long
romfs_read(int fd, void *buf, unsigned long len)
{
    struct romfs_fd *f;
    unsigned long remain, n;

    if (fd < 0 || fd >= ROMFS_MAX_OPEN || !romfs_fdtable[fd].used)
        return -1;
    f = &romfs_fdtable[fd];

    remain = f->entry->size - f->pos;
    n = len < remain ? len : remain;
    if (n > 0)
        memcpy(buf, romfs_base + f->entry->offset + f->pos, n);
    f->pos += n;
    return (long)n;
}

long
romfs_lseek(int fd, long offset, int whence)
{
    struct romfs_fd *f;
    long newpos;

    if (fd < 0 || fd >= ROMFS_MAX_OPEN || !romfs_fdtable[fd].used)
        return -1;
    f = &romfs_fdtable[fd];

    switch (whence) {
    case ROMFS_SEEK_SET: newpos = offset; break;
    case ROMFS_SEEK_CUR: newpos = (long)f->pos + offset; break;
    case ROMFS_SEEK_END: newpos = (long)f->entry->size + offset; break;
    default: return -1;
    }
    if (newpos < 0)
        newpos = 0;
    if ((unsigned long)newpos > f->entry->size)
        newpos = (long)f->entry->size;
    f->pos = (unsigned long)newpos;
    return newpos;
}

int
romfs_close(int fd)
{
    if (fd >= 0 && fd < ROMFS_MAX_OPEN)
        romfs_fdtable[fd].used = 0;
    return 0;
}

int
romfs_stat(const char *path, struct romfs_stat *out)
{
    const struct romfs_entry *e = romfs_find(path);
    if (!e)
        return -1;
    out->size = e->size;
    return 0;
}

const void *
romfs_data(const char *path, unsigned long *out_size)
{
    const struct romfs_entry *e = romfs_find(path);
    if (!e)
        return (const void *)0;
    if (out_size)
        *out_size = e->size;
    return romfs_base + e->offset;
}
