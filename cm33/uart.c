/*
 * uart.c - QEMU `mps2-an505`のCMSDK APB UART0(ポーリング送信のみ)。
 *
 * ベースアドレス0x40200000はQEMUの`info mtree`(モニタコマンド)実測で
 * 確認した(このボードのSSE-200 IoTKit拡張ペリフェラル領域にある
 * UART0)。RP2350実機のUARTはレジスタ配置・アドレスともに全く別物
 * (RP2350はARM PL011互換ではなくRaspberry Pi独自のUARTブロックを
 * 0x40070000/0x40078000に持つ)なので、Phase 7の残作業として実機用の
 * 差し替えが要る(TODO.md参照)。
 */
#include "uart.h"

#define UART0_BASE  0x40200000UL
#define UART0_DATA  (*(volatile unsigned int *)(UART0_BASE + 0x00))
#define UART0_STATE (*(volatile unsigned int *)(UART0_BASE + 0x04))
#define UART0_CTRL  (*(volatile unsigned int *)(UART0_BASE + 0x08))

#define UART0_STATE_TX_FULL 0x1u
#define UART0_CTRL_TX_EN    0x1u

void
picoperl_uart_putc(char c)
{
    UART0_CTRL |= UART0_CTRL_TX_EN;
    while (UART0_STATE & UART0_STATE_TX_FULL) { }
    UART0_DATA = (unsigned int)(unsigned char)c;
}

void
picoperl_uart_puts(const char *s)
{
    while (*s)
        picoperl_uart_putc(*s++);
}
