/*
 * syscalls.c - newlib(--specs=nano.specs --specs=nosys.specs)が
 * 未定義のまま残す下位のシステムコール群を、OS無しのベアメタル環境
 * 向けに最小限だけ実装する(Phase 7)。
 *
 * fd 1(stdout)/2(stderr)への_writeだけUART0に繋ぎ、それ以外
 * (_read/_close/_lseek/_fstat/_isatty等)はnosys.specs同様「対応する
 * ものが無い」ことを示すエラーを返す。ファイルI/Oそのものは
 * picoperlのlibc以下(env.c等)やromperlのvfs/ramfsが引き受ける領域で、
 * ここはあくまでnewlibのリンクを通すための最下層。
 *
 * _sbrkはnewlib自身の内部処理(printf内部のロケール/バッファ確保等)が
 * まれに使うmalloc用に、小さな固定サイズの静的領域からの単純な
 * bump allocatorとして用意する。Perl自身の大きなヒープ確保は
 * libc/malloc.c(picoperl_malloc)が別の固定ヒープ配列で独立に行う
 * ため、ここは小さくて足りる(TODO.md「Phase 7」のヒープ見積もり参照)。
 */
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include "uart.h"

#define SBRK_HEAP_SIZE (16UL * 1024UL)
static unsigned char sbrk_heap[SBRK_HEAP_SIZE];
static unsigned char *sbrk_ptr = sbrk_heap;

void *
_sbrk(int incr)
{
    unsigned char *prev = sbrk_ptr;
    if (sbrk_ptr + incr > sbrk_heap + SBRK_HEAP_SIZE) {
        errno = ENOMEM;
        return (void *)-1;
    }
    sbrk_ptr += incr;
    return prev;
}

int
_write(int fd, const char *buf, int len)
{
    int i;
    if (fd != 1 && fd != 2) {
        errno = EBADF;
        return -1;
    }
    for (i = 0; i < len; i++)
        picoperl_uart_putc(buf[i]);
    return len;
}

int
_read(int fd, char *buf, int len)
{
    (void)fd; (void)buf; (void)len;
    errno = ENOSYS;
    return -1;
}

int
_close(int fd)
{
    (void)fd;
    errno = ENOSYS;
    return -1;
}

off_t
_lseek(int fd, off_t offset, int whence)
{
    (void)fd; (void)offset; (void)whence;
    errno = ENOSYS;
    return -1;
}

int
_fstat(int fd, struct stat *st)
{
    (void)fd;
    st->st_mode = S_IFCHR;
    return 0;
}

int
_isatty(int fd)
{
    return (fd == 1 || fd == 2) ? 1 : 0;
}

void
_exit(int status)
{
    (void)status;
    while (1) { }
}

int
_kill(int pid, int sig)
{
    (void)pid; (void)sig;
    errno = ENOSYS;
    return -1;
}

int
_getpid(void)
{
    return 1;
}
