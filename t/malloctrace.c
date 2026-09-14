/*
 * malloctrace.c - LD_PRELOAD malloc/realloc トレーサ。
 *
 * romperlのpp_require直結(true zero-copy)経路が、実際にファイルサイズ
 * 分のmalloc/reallocを発生させていないかを実測で検証するために作った
 * デバッグツール。当初は使い捨てで/tmp配下に置いていたが、同種の検証を
 * 再現・自動化できるようt/以下にコミットし、test-zerocopy.shから使う。
 *
 * また、ROMFSセンチネルを";"1byteだけにしていた時に発生した
 * "Out of memory!"クラッシュ(realloc(ptr, (size_t)-28)という符号なし
 * 整数アンダーフロー)を発見したのも、このツールの原型を使ったLD_PRELOAD
 * トレースがきっかけだった(詳細はTODO.mdの「実装中に遭遇した重大バグ」
 * 参照)。
 *
 * 使い方:
 *   cc -shared -fPIC -o malloctrace.so malloctrace.c -ldl
 *   MALLOCTRACE_MIN=100 LD_PRELOAD=./malloctrace.so ./romperl -e '...'
 *
 * 環境変数:
 *   MALLOCTRACE_MIN  ログに出す最小サイズ(byte、デフォルト64)。
 *                    このサイズ未満のmalloc/reallocは無視する。
 *
 * 出力(stderr、1呼び出し1行):
 *   MALLOCTRACE malloc  size=<N> ptr=<P>
 *   MALLOCTRACE realloc size=<N> ptr=<OLD>-><NEW>
 *   MALLOCTRACE realloc UNDERFLOW size=<N> signed=<S> ptr=<OLD>-><NEW>
 *     (sizeをssize_tとして解釈すると負になる=符号なし整数アンダーフロー
 *      が起きている異常なreallocを別枠で警告する)
 */
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <dlfcn.h>

static void *(*real_malloc)(size_t) = NULL;
static void *(*real_realloc)(void *, size_t) = NULL;
static size_t min_size = 64;

static void
init(void)
{
    const char *env;

    real_malloc = (void *(*)(size_t))dlsym(RTLD_NEXT, "malloc");
    real_realloc = (void *(*)(void *, size_t))dlsym(RTLD_NEXT, "realloc");
    env = getenv("MALLOCTRACE_MIN");
    if (env)
        min_size = (size_t)strtoul(env, NULL, 10);
}

void *
malloc(size_t size)
{
    void *p;

    if (!real_malloc)
        init();
    p = real_malloc(size);
    if (size >= min_size)
        fprintf(stderr, "MALLOCTRACE malloc  size=%zu ptr=%p\n", size, p);
    return p;
}

void *
realloc(void *ptr, size_t size)
{
    void *p;

    if (!real_realloc)
        init();
    p = real_realloc(ptr, size);
    if ((long)size < 0) {
        fprintf(stderr,
            "MALLOCTRACE realloc UNDERFLOW size=%zu signed=%ld ptr=%p->%p\n",
            size, (long)size, ptr, p);
    } else if (size >= min_size) {
        fprintf(stderr, "MALLOCTRACE realloc size=%zu ptr=%p->%p\n",
            size, ptr, p);
    }
    return p;
}
