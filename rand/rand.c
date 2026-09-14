/*
 * rand.c - rand.h の実装。線形合同法(LCG)。
 *
 * 定数(1103515245, 12345)と下位ビットの捨て方(/65536して0x7FFFで
 * マスク)は、ANSI Cの規格書解説等でよく引用される「単純なrand()の
 * 実装例」そのもの。下位ビットの周期性が弱い(LCGの既知の弱点)ため、
 * `Drand01()`が結果をさらに`& 0x7FFF`する上位ビット寄りの使い方と
 * 相性が良い。
 */
#include "rand.h"

static unsigned long state = 1;

void
picoperl_srand(unsigned int seed)
{
    state = seed;
}

int
picoperl_rand(void)
{
    state = state * 1103515245UL + 12345UL;
    return (int)((state / 65536UL) % 32768UL);
}
