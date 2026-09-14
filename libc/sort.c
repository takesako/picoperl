/*
 * sort.c - sort.h の実装。挿入ソート。
 */
#include <stdlib.h>
#include <string.h>
#include "sort.h"

void
picoperl_qsort(void *base, size_t nmemb, size_t size,
               int (*compar)(const void *, const void *))
{
    unsigned char *arr = (unsigned char *)base;
    unsigned char *tmp;
    size_t i, j;

    if (nmemb < 2 || size == 0)
        return;

    tmp = (unsigned char *)malloc(size);
    if (!tmp)
        return; /* qsort(3)は失敗を報告する手段が無いので諦めるしかない */

    for (i = 1; i < nmemb; i++) {
        memcpy(tmp, arr + i * size, size);
        j = i;
        while (j > 0 && compar(arr + (j - 1) * size, tmp) > 0) {
            memcpy(arr + j * size, arr + (j - 1) * size, size);
            j--;
        }
        if (j != i)
            memcpy(arr + j * size, tmp, size);
    }

    free(tmp);
}
