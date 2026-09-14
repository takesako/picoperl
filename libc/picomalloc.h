/*
 * picomalloc.h - libcのmalloc(3)/calloc(3)/realloc(3)/free(3)に依存
 * しない、picoperl自前の固定ヒープアロケータ。
 *
 * ファイル名を素直に"malloc.h"にすると、romperlのビルド(-I../libc)で
 * `#include <malloc.h>`(GNU拡張の本物のヘッダ)がこのファイルと
 * 衝突しうる(picotime.h/picoctype.hで踏んだのと同じ罠を避けるため、
 * 最初から名前をずらしてある)。
 */
#ifndef PICOPERL_PICOMALLOC_H
#define PICOPERL_PICOMALLOC_H

#include <stddef.h>

void *picoperl_malloc(size_t size);
void *picoperl_calloc(size_t nmemb, size_t size);
void *picoperl_realloc(void *ptr, size_t size);
void  picoperl_free(void *ptr);

/* テスト用: ヒープの状態を調べる(picoperl自体は使わない)。 */
struct picoperl_malloc_stats {
    size_t heap_size;   /* ヒープ全体のバイト数 */
    size_t used;        /* 使用中チャンクのペイロード合計 */
    size_t free_total;  /* 空きチャンクのペイロード合計 */
    size_t free_chunks; /* 空きチャンクの個数(断片化の目安) */
};
void picoperl_malloc_stats(struct picoperl_malloc_stats *out);

/* テスト用: ヒープを解放して作り直す(テストケース間の独立性のため)。 */
void picoperl_malloc_reset(void);

#endif /* PICOPERL_PICOMALLOC_H */
