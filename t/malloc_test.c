/*
 * malloc.c(libcのmalloc/calloc/realloc/freeに依存しない固定ヒープ
 * アロケータ)の単体テスト。picoperl_malloc_stats()/picoperl_malloc_reset()
 * (テスト専用のフック)でヒープの内部状態を覗きながら検証する。
 */
#include <stdio.h>
#include <string.h>
#include "picomalloc.h"

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
    struct picoperl_malloc_stats st;
    void *p1, *p2, *p3;
    char *s;

    picoperl_malloc_reset();

    /* 基本: 確保して書き込んで読み出せる */
    p1 = picoperl_malloc(64);
    ok(p1 != NULL, "malloc(64) succeeds");
    memset(p1, 0xAB, 64);
    ok(((unsigned char *)p1)[0] == 0xAB && ((unsigned char *)p1)[63] == 0xAB,
        "the full requested region is writable");

    /* malloc(0)はNULLを返す(このシンプルな実装の仕様として) */
    ok(picoperl_malloc(0) == NULL, "malloc(0) returns NULL");

    /* calloc: ゼロ初期化される */
    p2 = picoperl_calloc(10, 8);
    ok(p2 != NULL, "calloc(10, 8) succeeds");
    {
        int allzero = 1, i;
        unsigned char *b = (unsigned char *)p2;
        for (i = 0; i < 80; i++)
            if (b[i] != 0)
                allzero = 0;
        ok(allzero, "calloc-ed memory is zeroed");
    }

    /* free してもう一度確保すると再利用される(ヒープが際限なく
     * 増えるだけではないことの簡単な確認) */
    picoperl_malloc_stats(&st);
    {
        size_t used_before = st.used;
        picoperl_free(p1);
        picoperl_malloc_stats(&st);
        ok(st.used < used_before, "free reduces the used total");
        p3 = picoperl_malloc(64);
        ok(p3 != NULL, "malloc after free succeeds again");
        ok(p3 == p1, "the freed block is reused (first-fit finds it)");
        picoperl_free(p3);
    }
    picoperl_free(p2);

    /* 全部freeした後は、ヒープ全体が1つの空きチャンクに戻っている
     * (前方/後方の合体が正しく効いていることの確認)。 */
    picoperl_malloc_stats(&st);
    ok(st.used == 0, "everything is freed: used == 0");
    ok(st.free_chunks == 1, "everything coalesces back into a single free chunk");
    /* ヘッダのオーバーヘッド分だけ小さいはずだが、その正確なバイト数は
     * malloc.c内部の実装詳細なので、余裕を持った範囲でだけ確認する。 */
    ok(st.free_total <= st.heap_size && st.free_total > st.heap_size - 64,
        "free_total accounts for nearly the whole heap (minus one small header)");

    /* 断片化してから解放する順序を変えても合体すること
     * (前方合体だけでなく後方合体も効くことを検証する)。 */
    picoperl_malloc_reset();
    {
        void *a = picoperl_malloc(100);
        void *b = picoperl_malloc(100);
        void *c = picoperl_malloc(100);
        picoperl_free(a);
        picoperl_free(c);
        picoperl_malloc_stats(&st);
        ok(st.free_chunks == 2, "freeing the outer two (non-adjacent-in-time) blocks leaves 2 free chunks");
        picoperl_free(b); /* bはaとcの間にある。両側と合体できるはず */
        picoperl_malloc_stats(&st);
        ok(st.free_chunks == 1, "freeing the middle block merges with both neighbors (forward and backward coalescing)");
    }

    /* realloc: 拡大時は内容が保存される */
    picoperl_malloc_reset();
    s = (char *)picoperl_malloc(5);
    memcpy(s, "abcd", 5);
    s = (char *)picoperl_realloc(s, 100);
    ok(s != NULL, "realloc to a larger size succeeds");
    ok(strcmp(s, "abcd") == 0, "realloc preserves the original content");
    picoperl_free(s);

    /* realloc: 縮小(既存サイズ以下)はポインタを使い回す */
    picoperl_malloc_reset();
    s = (char *)picoperl_malloc(100);
    {
        void *same = picoperl_realloc(s, 10);
        ok(same == s, "realloc to a smaller size reuses the same pointer");
    }
    picoperl_free(s);

    /* realloc(NULL, n) は malloc(n) と同じ */
    picoperl_malloc_reset();
    p1 = picoperl_realloc(NULL, 32);
    ok(p1 != NULL, "realloc(NULL, n) behaves like malloc(n)");
    picoperl_free(p1);

    /* realloc(p, 0) は free(p) と同じで NULL を返す */
    p1 = picoperl_malloc(32);
    ok(picoperl_realloc(p1, 0) == NULL, "realloc(p, 0) returns NULL");
    picoperl_malloc_stats(&st);
    ok(st.used == 0, "realloc(p, 0) actually frees the block");

    /* out of memory: ヒープより大きい要求は失敗する */
    picoperl_malloc_reset();
    picoperl_malloc_stats(&st);
    ok(picoperl_malloc(st.heap_size * 2) == NULL, "an allocation larger than the whole heap fails cleanly");

    /* free(NULL)は安全にno-op */
    picoperl_free(NULL);
    ok(1, "free(NULL) doesn't crash");

    if (failures) {
        printf("FAILED %d\n", failures);
        return 1;
    }
    printf("ALL TESTS PASSED\n");
    return 0;
}
