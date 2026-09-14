/*
 * sort.c(libcのqsort(3)に依存しない挿入ソート)の単体テスト。
 */
#include <stdio.h>
#include <string.h>
#include "sort.h"

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

static int
int_cmp(const void *a, const void *b)
{
    int ia = *(const int *)a, ib = *(const int *)b;
    return ia - ib;
}

static int
is_sorted_ints(const int *a, size_t n)
{
    size_t i;
    for (i = 1; i < n; i++)
        if (a[i - 1] > a[i])
            return 0;
    return 1;
}

struct pair {
    int key;
    int orig_index; /* 安定性(stability)確認用 */
};

static int
pair_cmp(const void *a, const void *b)
{
    return ((const struct pair *)a)->key - ((const struct pair *)b)->key;
}

int
main(void)
{
    {
        int a[] = { 5, 3, 1, 4, 2 };
        picoperl_qsort(a, 5, sizeof(int), int_cmp);
        ok(is_sorted_ints(a, 5) && a[0] == 1 && a[4] == 5, "sorts a small int array");
    }

    {
        int a[] = { 1 };
        picoperl_qsort(a, 1, sizeof(int), int_cmp);
        ok(a[0] == 1, "single-element array is a no-op");
    }

    {
        int a[2] = { 9, 9 }; /* 内容は使わない、0要素呼び出しがクラッシュしないことだけ確認 */
        picoperl_qsort(a, 0, sizeof(int), int_cmp);
        ok(1, "nmemb=0 doesn't crash");
    }

    {
        int a[] = { 1, 2, 3, 4, 5 };
        picoperl_qsort(a, 5, sizeof(int), int_cmp);
        ok(is_sorted_ints(a, 5), "already-sorted input stays sorted");
    }

    {
        int a[] = { 5, 4, 3, 2, 1 };
        picoperl_qsort(a, 5, sizeof(int), int_cmp);
        ok(is_sorted_ints(a, 5) && a[0] == 1, "reverse-sorted input gets sorted");
    }

    {
        int a[] = { 3, 1, 3, 2, 3, 1 };
        picoperl_qsort(a, 6, sizeof(int), int_cmp);
        ok(is_sorted_ints(a, 6), "duplicate keys are handled");
    }

    {
        /* 安定性: 同じkeyの要素同士は元の相対順序を保つ */
        struct pair a[] = {
            { 2, 0 }, { 1, 1 }, { 2, 2 }, { 1, 3 }, { 2, 4 }
        };
        picoperl_qsort(a, 5, sizeof(struct pair), pair_cmp);
        ok(a[0].key == 1 && a[1].key == 1 && a[0].orig_index == 1 && a[1].orig_index == 3,
            "stable sort: key==1 elements keep original relative order");
        ok(a[2].key == 2 && a[3].key == 2 && a[4].key == 2 &&
           a[2].orig_index == 0 && a[3].orig_index == 2 && a[4].orig_index == 4,
            "stable sort: key==2 elements keep original relative order");
    }

    {
        /* 呼び出し元(op.cのtr///コンパイル)と同じ「要素サイズが
         * int以外」のケース: UVペア(2*sizeof(long)相当)を模した構造体。 */
        struct { long lo, hi; } a[] = {
            { 30, 39 }, { 10, 19 }, { 20, 29 }
        };
        int cmp(const void *x, const void *y) {
            long lx = ((const long *)x)[0], ly = ((const long *)y)[0];
            return (lx > ly) - (lx < ly);
        }
        picoperl_qsort(a, 3, sizeof(a[0]), cmp);
        ok(a[0].lo == 10 && a[1].lo == 20 && a[2].lo == 30,
            "sorts a wider element type (mirrors op.c's tr/// UV-pair usage)");
    }

    if (failures) {
        printf("FAILED %d\n", failures);
        return 1;
    }
    printf("ALL TESTS PASSED\n");
    return 0;
}
