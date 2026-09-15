/*
 * startup.c - Cortex-M33ベクタテーブルとReset_Handler(Phase 7)。
 *
 * ARMv8-M(Cortex-M33)のベクタテーブルはv7-M(Cortex-M3/M4)と要素数が
 * 違う点に注意: index 7に「SecureFault」が新設されている。ここを
 * v7-M用のテーブル(indexをそのまま流用してreservedのまま0にする)を
 * 書くと、SecureFaultが発生した際にNULLへジャンプして即クラッシュする
 * (実際にこの間違いを一度やって再現・特定した)。
 */
#include <stdint.h>

extern unsigned long _estack;
extern unsigned long _sdata, _edata, _sidata;
extern unsigned long _sbss, _ebss;

void Reset_Handler(void);
void Default_Handler(void);

__attribute__((section(".isr_vector")))
void (* const picoperl_vector_table[])(void) = {
    (void (*)(void))&_estack, /*  0: initial SP */
    Reset_Handler,            /*  1: Reset */
    Default_Handler,          /*  2: NMI */
    Default_Handler,          /*  3: HardFault */
    Default_Handler,          /*  4: MemManage */
    Default_Handler,          /*  5: BusFault */
    Default_Handler,          /*  6: UsageFault */
    Default_Handler,          /*  7: SecureFault (ARMv8-Mで新設) */
    0,                        /*  8: reserved */
    0,                        /*  9: reserved */
    0,                        /* 10: reserved */
    Default_Handler,          /* 11: SVCall */
    Default_Handler,          /* 12: DebugMonitor */
    0,                        /* 13: reserved */
    Default_Handler,          /* 14: PendSV */
    Default_Handler,          /* 15: SysTick */
};

void Default_Handler(void)
{
    while (1) { }
}

void Reset_Handler(void)
{
    unsigned long *src, *dst;

    src = &_sidata;
    dst = &_sdata;
    while (dst < &_edata)
        *dst++ = *src++;

    dst = &_sbss;
    while (dst < &_ebss)
        *dst++ = 0;

    extern int main(void);
    main();
    while (1) { }
}
