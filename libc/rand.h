/*
 * rand.h - libcのrand(3)/srand(3)に依存しない、picoperl自前のPRNG。
 *
 * Perlの`rand`/`srand`組み込み関数(pp.cのpp_rand/pp_srand)は、
 * uconfig.hの`Drand01()`/`seedDrand01()`マクロ経由でlibcのrand()/
 * srand()を直接呼ぶだけの実装になっている
 * (`Drand01() = (rand() & 0x7FFF) / (double)(1<<15)`,
 *  `seedDrand01(x) = srand((Rand_seed_t)x)`)。Perlの乱数は暗号用途では
 * ないため、複雑なPRNGは不要。ANSI Cの教科書でよく使われる単純な線形
 * 合同法(LCG)で、RAND_MAX=32767(0x7FFF)相当の値を返す。
 */
#ifndef PICOPERL_RAND_H
#define PICOPERL_RAND_H

void picoperl_srand(unsigned int seed);
int  picoperl_rand(void);

#endif /* PICOPERL_RAND_H */
