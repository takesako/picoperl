/*
 * test-malloc.c - libc/malloc.cの`-DPICOPERL_MALLOC_STATIC_HEAP`版
 * (静的配列バックエンド)がCortex-M33実機相当(QEMU `mps2-an505`)でも
 * 正しく動くことを確認する。x86_64版の`t/malloc_test.c`の要点だけを
 * UART出力で確認する簡略版(newlibのassert/exit系に頼らない)。
 */
#include <string.h>
#include "uart.h"
#include "../libc/picomalloc.h"

static int failed = 0;

static void
check(int cond, const char *name)
{
    if (cond) {
        picoperl_uart_puts("ok ");
    } else {
        picoperl_uart_puts("NG ");
        failed = 1;
    }
    picoperl_uart_puts(name);
    picoperl_uart_putc('\n');
}

int
main(void)
{
    void *a, *b, *c;
    struct picoperl_malloc_stats st;

    picoperl_malloc_reset();

    a = picoperl_malloc(64);
    check(a != NULL, "malloc(64) on static heap succeeds");
    memset(a, 0x5a, 64);

    b = picoperl_calloc(8, 8);
    check(b != NULL, "calloc(8,8) on static heap succeeds");
    check(((unsigned char *)b)[0] == 0 && ((unsigned char *)b)[63] == 0,
          "calloc-ed memory is zeroed");

    picoperl_free(a);
    c = picoperl_malloc(64);
    check(c != NULL, "malloc after free succeeds again");

    picoperl_free(b);
    picoperl_free(c);
    picoperl_malloc_stats(&st);
    check(st.used == 0, "everything freed: used == 0 on static heap");

    if (failed)
        picoperl_uart_puts("SOME TESTS FAILED\n");
    else
        picoperl_uart_puts("ALL TESTS PASSED\n");

    return failed;
}
