/*
 * rand.c(libcのrand/srandに依存しないPRNG)の単体テスト。
 */
#include <stdio.h>
#include "rand.h"

static int failures = 0;

static void
ok(int cond, const char *name)
{
    if (cond) {
        printf("ok %s\n", name);
    } else {
        printf("NG %s\n", name);
        failures++;
    }
}

int
main(void)
{
    int i;
    int all_in_range = 1;
    int all_same = 1;
    int first_seed1;
    int seq_a[10], seq_b[10];

    picoperl_srand(1);
    first_seed1 = picoperl_rand();

    for (i = 0; i < 1000; i++) {
        int v = picoperl_rand();
        if (v < 0 || v > 32767)
            all_in_range = 0;
        if (v != first_seed1)
            all_same = 0;
    }
    ok(all_in_range, "rand() always returns a value in [0, 32767]");
    ok(!all_same, "rand() doesn't just repeat the first value forever");

    /* 同じseedなら同じ列を再現する(決定的) */
    picoperl_srand(42);
    for (i = 0; i < 10; i++)
        seq_a[i] = picoperl_rand();
    picoperl_srand(42);
    for (i = 0; i < 10; i++)
        seq_b[i] = picoperl_rand();
    {
        int same = 1;
        for (i = 0; i < 10; i++)
            if (seq_a[i] != seq_b[i])
                same = 0;
        ok(same, "srand(same seed) reproduces the same sequence");
    }

    /* 違うseedなら(ほぼ確実に)違う最初の値になる */
    {
        int v1, v2;
        picoperl_srand(1);
        v1 = picoperl_rand();
        picoperl_srand(2);
        v2 = picoperl_rand();
        ok(v1 != v2, "different seeds produce different sequences");
    }

    if (failures) {
        printf("FAILED %d\n", failures);
        return 1;
    }
    printf("ALL TESTS PASSED\n");
    return 0;
}
