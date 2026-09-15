/* link.ld - Cortex-M33ベアメタル向けリンカスクリプト(Phase 7)。
 *
 * 今のところQEMU `mps2-an505`(SSE-200ベース、Cortex-M33 x1)専用。
 * RP2350実機はこれとは全く異なるメモリマップ(XIP flash 0x10000000
 * オフセットで最大32MB、SRAM 0x20000000、しかもRP2350のブートROMは
 * 単純な「先頭にベクタテーブル」ではなく専用の署名付きメタデータ
 * ブロックを要求する)なので、このリンカスクリプトはそのままでは
 * RP2350では動かない。TODO.md「Phase 7」参照。
 *
 * ORIGIN=0x10000000であって0x00000000ではない点に注意: mps2-an505は
 * ARMv8-M(TrustZone)ボードで、CPUはSecure状態でリセットする。QEMUの
 * `-kernel`はELFのアドレスをそのまま物理メモリに書き込むが、Secure状態の
 * CPUが実際にベクタテーブルを読みに行くアドレスは「Secureエイリアス」
 * である0x10000000側(0x00000000側はNon-secureエイリアス)。0x00000000に
 * 置くと、CPUリセット時にSP=0/PC=0を読んでしまい即座にUsageFault→
 * (スタックが無いので)BusFault→エスカレーション失敗でロックアップする
 * (実際にこの現象を再現・特定した上で0x10000000に変更した)。
 */
MEMORY
{
    RAM (rwx) : ORIGIN = 0x10000000, LENGTH = 4M
}

_estack = ORIGIN(RAM) + LENGTH(RAM);

SECTIONS
{
    .isr_vector : {
        KEEP(*(.isr_vector))
    } > RAM

    .text : {
        *(.text*)
        *(.rodata*)
    } > RAM

    _sidata = LOADADDR(.data);
    .data : {
        _sdata = .;
        *(.data*)
        _edata = .;
    } > RAM

    .bss : {
        _sbss = .;
        *(.bss*)
        *(COMMON)
        _ebss = .;
    } > RAM
}
