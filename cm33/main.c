/*
 * main.c - Phase 7の最初の検証用エントリポイント。
 *
 * まだpicoperl本体はリンクしていない(TODO.md「Phase 7」の残作業
 * 参照)。ここではベクタテーブル/リンカスクリプト/スタートアップ/
 * UART/newlibリターゲットの組み合わせがCortex-M33実機相当の
 * (QEMU `mps2-an505`上の)エミュレーションで実際に動くことだけを
 * 確認する。
 */
#include <stdio.h>
#include "uart.h"

int
main(void)
{
    picoperl_uart_puts("hello cortex-m33 from qemu (direct UART)\n");
    printf("hello cortex-m33 from qemu (printf via newlib+_write)\n");
    return 0;
}
