/*
 * malloc.c - picomalloc.h の実装。
 *
 * ヒープ領域全体を単一の連続領域として確保し、その中をチャンク単位で
 * 切り出す最小限のアロケータ。x86_64ホストでは本物のmalloc()で
 * ヒープ領域を一括確保し(init_heap参照)、その中を細かく切り出す。
 * 実機(RP2350、Phase 7)ではこの一括確保部分を固定サイズの静的配列
 * (またはリンカスクリプトで確保した領域)に差し替える想定。
 *
 * データ構造(性能よりシンプルさを優先): ヒープ内の全チャンク
 * (使用中・空き問わず)を、物理的なアドレス順の単方向リストとして
 * 繋ぐ(chunk->nextは「次に確保したチャンク」ではなく「メモリ上で
 * 直後にあるチャンク」を指す)。各チャンクのヘッダはヒープ自身の中に
 * 埋め込み、別に管理領域は持たない。
 *
 *   malloc: 空きチャンクをfirst-fitで探し、十分大きければ分割する。
 *   free:   対象をfreeにした上で、
 *           - 前方(chunk->next)が空きなら合体する(O(1)、リストで
 *             直接繋がっているため)
 *           - 後方(1つ前の物理チャンク)が空きなら合体する(O(n)、
 *             単方向リストのため先頭から線形探索して前のチャンクを
 *             見つける必要がある。速度は要求されていないため、
 *             双方向リストにする複雑さよりこちらを選んだ)
 *   realloc: 常に新規mallocしてコピーし、元をfreeする単純な実装
 *            (新しいサイズが元のサイズ以下ならそのまま使い回す)。
 */
#include <stdlib.h>
#include <string.h>
#include "picomalloc.h"

/*
 * ヒープサイズ。当初RP2350のSRAM(520KB)を想定して仮置きしたが、
 * 実測したところPerl 5.12.5(NO_MATHOMS/float NV/lib無し等、既存の
 * 削減を全て適用した上でも)のメモリ使用量はそれを大きく超えることが
 * 判明した:
 *   - `use strict; use warnings; use Carp;`だけでも384KB前後で
 *     ぎりぎりになり、512KB付近でもクラッシュ(後述)する
 *   - `use CGI; CGI->new->header/start_html/end_html`のような、
 *     やや本格的な処理には2MB以上必要
 * つまり現状のPerl 5.12.5では、RP2350の520KB(SRAM全体)には収まる
 * スクリプトはほぼ無い、というのが実測に基づく結論。ここでは既存の
 * テスト一式(CGI.pm使用を含む)が安定して通る4MBを暫定値としている。
 * RP2350実機で動かすには、Phase 7で埋め込みモジュール(CGI.pm等)を
 * 削るか、Perl側のメモリ使用量そのものをさらに削減するか、外部RAMを
 * 使うか、といった追加の検討が要る。
 *
 * **重大な発見**: 384KB〜512KB付近の狭い範囲で、"Out of memory!"の
 * 代わりにSEGVでクラッシュすることがある。原因を`gdb`で追ったところ、
 * これはこのアロケータのバグではなく、Perl 5.12.5のop.c内
 * `Perl_Slab_Alloc`(構文木のノードを確保する専用アロケータ)が
 * `PerlMemShared_calloc()`(=`calloc()`、このアロケータへリダイレクト
 * 済み)失敗時に`NULL`を返すだけで、`Perl_safesysmalloc`(通常の
 * malloc経路)のように"Out of memory!"を出して`my_exit()`する処理を
 * 持たないことが原因だった。呼び出し元(`perly.y`の自動生成パーサ、
 * 例えば`Perl_newWHILEOP`)側もこの`NULL`を全箇所ではチェックしておらず、
 * 結果として`loop->op_type = ...`のようなNULLポインタ経由の書き込みで
 * 落ちる(`gdb`で`loop=0x0`のままop.c:4998に到達したことを確認済み)。
 * 通常の環境ではmalloc/callocが仮想メモリに支えられ実質的に失敗しない
 * ため、この`Perl_Slab_Alloc`の「NULLチェック漏れ」は表面化しない
 * 潜在的な弱点だったが、本当に有限なヒープを使う今回、初めて実際に
 * 踏んだ。picoperl-5.12.5自体は変更しない方針のため、ヒープに
 * 十分な余裕を持たせることで回避している(この種の失敗はこのアロケータ
 * を直しても解決しない、Perl core側の限界であるため)。
 */
#define PICOPERL_HEAP_SIZE (4096UL * 1024UL)

#define PICOPERL_MALLOC_ALIGN 8
#define PICOPERL_MIN_PAYLOAD  8

struct chunk {
    size_t size;         /* ペイロードのバイト数(ヘッダ自体は含まない) */
    struct chunk *next;  /* メモリ上で直後にあるチャンク(NULLなら末端) */
    int free;
};

static unsigned char *heap_base = NULL;
static struct chunk *heap_head = NULL;

static size_t
align_up(size_t n)
{
    return (n + (PICOPERL_MALLOC_ALIGN - 1)) & ~(size_t)(PICOPERL_MALLOC_ALIGN - 1);
}

static void
init_heap(void)
{
    if (heap_head)
        return;

    /*
     * `(malloc)`と余分な括弧で囲っているのは、libc/stdlib.hのシムが
     * malloc()自体をpicoperl_malloc()にリダイレクトしているため、
     * 素の`malloc(...)`と書くと自分自身を再帰呼び出ししてしまうのを
     * 防ぐため(関数マクロは「識別子の直後に'('」が展開条件のため、
     * `(malloc)`のように直後が'('でなければ展開されず本物のシンボルを
     * 指す)。ここが「x86_64では本物のmallocでヒープ領域を一括確保する」
     * 部分そのもの。
     */
    heap_base = (unsigned char *)(malloc)(PICOPERL_HEAP_SIZE);
    if (!heap_base)
        return; /* 確保に失敗したら以後ずっとメモリ不足として振る舞う */

    heap_head = (struct chunk *)heap_base;
    heap_head->size = PICOPERL_HEAP_SIZE - sizeof(struct chunk);
    heap_head->next = NULL;
    heap_head->free = 1;
}

void
picoperl_malloc_reset(void)
{
    if (heap_base)
        (free)(heap_base);
    heap_base = NULL;
    heap_head = NULL;
    init_heap();
}

void *
picoperl_malloc(size_t n)
{
    struct chunk *c, *newc;
    size_t need;

    init_heap();
    if (n == 0 || !heap_head)
        return NULL;
    need = align_up(n);

    for (c = heap_head; c; c = c->next) {
        if (!c->free || c->size < need)
            continue;

        if (c->size >= need + sizeof(struct chunk) + PICOPERL_MIN_PAYLOAD) {
            /* 余りが十分大きい時だけ分割する。細切れの空きチャンクが
             * 際限なく増えるのを防ぐため。 */
            newc = (struct chunk *)((unsigned char *)(c + 1) + need);
            newc->size = c->size - need - sizeof(struct chunk);
            newc->next = c->next;
            newc->free = 1;
            c->next = newc;
            c->size = need;
        }
        c->free = 0;
        return (void *)(c + 1);
    }
    return NULL; /* out of memory */
}

void *
picoperl_calloc(size_t nmemb, size_t size)
{
    void *p;
    size_t total;

    if (nmemb != 0 && size > (size_t)-1 / nmemb)
        return NULL; /* nmemb*sizeのオーバーフロー */
    total = nmemb * size;
    p = picoperl_malloc(total);
    if (p)
        memset(p, 0, total);
    return p;
}

void
picoperl_free(void *ptr)
{
    struct chunk *c, *prev, *p;

    if (!ptr)
        return;
    c = (struct chunk *)ptr - 1;
    c->free = 1;

    /* 前方合体: chunk->nextは物理的に直後のチャンクなのでO(1)で確認できる。 */
    while (c->next && c->next->free) {
        c->size += sizeof(struct chunk) + c->next->size;
        c->next = c->next->next;
    }

    /* 後方合体: 単方向リストなので先頭から線形探索して1つ前を見つける
     * (速度は要求されていないため、この単純さを優先した)。 */
    prev = NULL;
    for (p = heap_head; p && p != c; p = p->next)
        prev = p;
    if (prev && prev->free) {
        prev->size += sizeof(struct chunk) + c->size;
        prev->next = c->next;
    }
}

void *
picoperl_realloc(void *ptr, size_t n)
{
    struct chunk *c;
    void *newptr;
    size_t oldsize;

    if (!ptr)
        return picoperl_malloc(n);
    if (n == 0) {
        picoperl_free(ptr);
        return NULL;
    }

    c = (struct chunk *)ptr - 1;
    oldsize = c->size;
    if (align_up(n) <= oldsize)
        return ptr; /* 既に十分大きい。縮小や分割はしない単純な実装 */

    newptr = picoperl_malloc(n);
    if (!newptr)
        return NULL;
    memcpy(newptr, ptr, oldsize);
    picoperl_free(ptr);
    return newptr;
}

void
picoperl_malloc_stats(struct picoperl_malloc_stats *out)
{
    struct chunk *c;

    if (!out)
        return;
    init_heap();
    out->heap_size = PICOPERL_HEAP_SIZE;
    out->used = 0;
    out->free_total = 0;
    out->free_chunks = 0;
    for (c = heap_head; c; c = c->next) {
        if (c->free) {
            out->free_total += c->size;
            out->free_chunks++;
        } else {
            out->used += c->size;
        }
    }
}
