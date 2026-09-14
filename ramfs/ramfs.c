/*
 * ramfs.c - ramfs.h の実装。
 *
 * データ構造はフラットな単方向リスト(ファイル数の上限を決め打ちしない
 * ため)。書き込みのたびに realloc でちょうど必要なサイズまで伸ばすだけで、
 * 倍々確保のような先読み最適化はしない(性能よりシンプルさを優先する
 * という方針のため)。
 */
#include <stdlib.h>
#include <string.h>
#include "ramfs.h"

struct ramfs_inode {
    char name[RAMFS_NAME_MAX];
    unsigned char *data;   /* mallocされた本体。sizeちょうどの長さ */
    unsigned long size;
    struct ramfs_inode *next;
};

struct ramfs_FILE {
    struct ramfs_inode *inode;
    unsigned long pos;
    int readable;
    int writable;
    int append;   /* 'a'/'a+': 書き込みは常に末尾に行われる */
    int eof;
};

static struct ramfs_inode *inode_list = NULL;

static struct ramfs_inode *
find_inode(const char *name)
{
    struct ramfs_inode *p;

    for (p = inode_list; p; p = p->next) {
        if (strcmp(p->name, name) == 0)
            return p;
    }
    return NULL;
}

static struct ramfs_inode *
create_inode(const char *name)
{
    struct ramfs_inode *node;

    node = (struct ramfs_inode *)malloc(sizeof *node);
    if (!node)
        return NULL;
    strcpy(node->name, name);
    node->data = NULL;
    node->size = 0;
    node->next = inode_list;
    inode_list = node;
    return node;
}

void
ramfs_init(void)
{
    struct ramfs_inode *p, *next;

    for (p = inode_list; p; p = next) {
        next = p->next;
        free(p->data);
        free(p);
    }
    inode_list = NULL;
}

ramfs_FILE *
ramfs_fopen(const char *name, const char *mode)
{
    int do_read = 0, do_write = 0, do_append = 0, do_create = 0, do_trunc = 0;
    struct ramfs_inode *inode;
    ramfs_FILE *fp;

    if (!name || !mode || strlen(name) >= RAMFS_NAME_MAX)
        return NULL;

    switch (mode[0]) {
    case 'r':
        do_read = 1;
        if (mode[1] == '+')
            do_write = 1;
        break;
    case 'w':
        do_write = 1;
        do_create = 1;
        do_trunc = 1;
        if (mode[1] == '+')
            do_read = 1;
        break;
    case 'a':
        do_write = 1;
        do_append = 1;
        do_create = 1;
        if (mode[1] == '+')
            do_read = 1;
        break;
    default:
        return NULL;
    }

    inode = find_inode(name);
    if (!inode) {
        if (!do_create)
            return NULL;
        inode = create_inode(name);
        if (!inode)
            return NULL;
    } else if (do_trunc) {
        free(inode->data);
        inode->data = NULL;
        inode->size = 0;
    }

    fp = (ramfs_FILE *)malloc(sizeof *fp);
    if (!fp)
        return NULL;
    fp->inode = inode;
    fp->pos = do_append ? inode->size : 0;
    fp->readable = do_read;
    fp->writable = do_write;
    fp->append = do_append;
    fp->eof = 0;
    return fp;
}

unsigned long
ramfs_fread(void *ptr, unsigned long size, unsigned long nmemb, ramfs_FILE *fp)
{
    unsigned long total, avail, copy_bytes, nmemb_done;

    if (!fp || !fp->readable || size == 0 || nmemb == 0)
        return 0;

    total = size * nmemb;
    avail = (fp->pos < fp->inode->size) ? (fp->inode->size - fp->pos) : 0;
    copy_bytes = (total < avail) ? total : avail;
    nmemb_done = copy_bytes / size;
    copy_bytes = nmemb_done * size;

    if (copy_bytes > 0)
        memcpy(ptr, fp->inode->data + fp->pos, copy_bytes);
    fp->pos += copy_bytes;
    if (nmemb_done < nmemb)
        fp->eof = 1;
    return nmemb_done;
}

unsigned long
ramfs_fwrite(const void *ptr, unsigned long size, unsigned long nmemb, ramfs_FILE *fp)
{
    unsigned long total, need;
    unsigned char *newdata;

    if (!fp || !fp->writable || size == 0 || nmemb == 0)
        return 0;

    total = size * nmemb;

    if (fp->append)
        fp->pos = fp->inode->size;

    need = fp->pos + total;
    if (need > fp->inode->size) {
        newdata = (unsigned char *)realloc(fp->inode->data, need);
        if (!newdata)
            return 0;
        if (fp->pos > fp->inode->size)
            memset(newdata + fp->inode->size, 0, fp->pos - fp->inode->size);
        fp->inode->data = newdata;
        fp->inode->size = need;
    }

    memcpy(fp->inode->data + fp->pos, ptr, total);
    fp->pos += total;
    fp->eof = 0;
    return nmemb;
}

int
ramfs_fseek(ramfs_FILE *fp, long offset, int whence)
{
    long base, newpos;

    if (!fp)
        return -1;

    switch (whence) {
    case RAMFS_SEEK_SET: base = 0; break;
    case RAMFS_SEEK_CUR: base = (long)fp->pos; break;
    case RAMFS_SEEK_END: base = (long)fp->inode->size; break;
    default: return -1;
    }

    newpos = base + offset;
    if (newpos < 0)
        return -1;

    fp->pos = (unsigned long)newpos;
    fp->eof = 0;
    return 0;
}

long
ramfs_ftell(ramfs_FILE *fp)
{
    if (!fp)
        return -1;
    return (long)fp->pos;
}

int
ramfs_feof(ramfs_FILE *fp)
{
    return fp ? fp->eof : 0;
}

int
ramfs_fclose(ramfs_FILE *fp)
{
    if (!fp)
        return -1;
    free(fp);
    return 0;
}

int
ramfs_remove(const char *name)
{
    struct ramfs_inode *p, *prev = NULL;

    if (!name)
        return -1;

    for (p = inode_list; p; prev = p, p = p->next) {
        if (strcmp(p->name, name) == 0) {
            if (prev)
                prev->next = p->next;
            else
                inode_list = p->next;
            free(p->data);
            free(p);
            return 0;
        }
    }
    return -1;
}

int
ramfs_stat(const char *name, struct ramfs_stat *st)
{
    struct ramfs_inode *inode;

    if (!name)
        return -1;
    inode = find_inode(name);
    if (!inode)
        return -1;
    if (st)
        st->size = inode->size;
    return 0;
}
