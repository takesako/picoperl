/*
 * vfs.c(romfsとramfsを束ねるディスパッチ層)の単体テスト。
 * まだPerl/PerlIOには一切繋いでおらず、vfs.hのAPIだけをCから直接
 * たたいて「読み取りはramfs優先・romfsフォールバック」「書き込みは
 * 常にramfsのみ」という設計方針どおりに動くことを検証する。
 * vfs.h/ramfs.h/romfs.h は ../vfs/ に -I. で通す前提(vfs/Makefile参照)。
 * romfsイメージは vfs/testdata/readonly.txt (内容: "from romfs\n") 1つ
 * だけを埋め込んだ vfs_test_romfs_image[] を使う。
 */
#include <stdio.h>
#include <string.h>
#include "vfs.h"
#include "ramfs.h"
#include "romfs.h"

extern const unsigned char vfs_test_romfs_image[];
extern const unsigned long vfs_test_romfs_image_size;

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
    vfs_FILE *fp;
    char buf[64];
    struct vfs_stat st;
    unsigned long n;

    ok(romfs_init(vfs_test_romfs_image, vfs_test_romfs_image_size), "romfs_init");
    vfs_init();

    /* --- 存在しないファイル: どちらにも無ければ失敗 --- */
    ok(vfs_fopen("nowhere.txt", "r") == NULL, "read-only open of unknown name fails");
    ok(vfs_stat("nowhere.txt", &st) == -1, "stat of unknown name fails");

    /* --- romfsだけに存在するファイルの読み取りフォールバック --- */
    fp = vfs_fopen("readonly.txt", "r");
    ok(fp != NULL, "fopen r falls back to romfs when not in ramfs");
    memset(buf, 0, sizeof(buf));
    n = vfs_fread(buf, 1, sizeof(buf) - 1, fp);
    ok(n == strlen("from romfs\n") && strcmp(buf, "from romfs\n") == 0,
        "content read via romfs fallback matches");
    ok(vfs_fwrite("x", 1, 1, fp) == 0, "write on a romfs-backed handle is rejected");
    vfs_fclose(fp);
    ok(vfs_stat("readonly.txt", &st) == 0 && st.size == strlen("from romfs\n"),
        "stat falls back to romfs size");

    /* --- ramfsだけに新規作成するファイル(romfsと無関係) --- */
    fp = vfs_fopen("new.txt", "w");
    ok(fp != NULL, "fopen w creates a new ramfs-only file");
    vfs_fwrite("hello", 1, 5, fp);
    vfs_fclose(fp);
    fp = vfs_fopen("new.txt", "r");
    memset(buf, 0, sizeof(buf));
    n = vfs_fread(buf, 1, sizeof(buf) - 1, fp);
    ok(n == 5 && strcmp(buf, "hello") == 0, "newly created file reads back from ramfs");
    vfs_fclose(fp);

    /* r+ で部分更新 */
    fp = vfs_fopen("new.txt", "r+");
    ok(fp != NULL, "fopen r+ on ramfs file");
    vfs_fwrite("HE", 1, 2, fp);
    vfs_fclose(fp);
    fp = vfs_fopen("new.txt", "r");
    memset(buf, 0, sizeof(buf));
    vfs_fread(buf, 1, sizeof(buf) - 1, fp);
    ok(strcmp(buf, "HEllo") == 0, "r+ updates ramfs file in place");
    vfs_fclose(fp);

    /* --- romfsと同名のファイルをwriteすると、以後の読み取りはramfs優先になる (shadow) --- */
    fp = vfs_fopen("readonly.txt", "w");
    ok(fp != NULL, "fopen w on a name that also exists in romfs succeeds");
    vfs_fwrite("shadowed", 1, 8, fp);
    vfs_fclose(fp);
    fp = vfs_fopen("readonly.txt", "r");
    memset(buf, 0, sizeof(buf));
    n = vfs_fread(buf, 1, sizeof(buf) - 1, fp);
    ok(n == 8 && strcmp(buf, "shadowed") == 0,
        "ramfs copy shadows the romfs original for reads");
    vfs_fclose(fp);
    ok(vfs_stat("readonly.txt", &st) == 0 && st.size == 8, "stat also sees the shadowing ramfs copy");

    /* --- shadowを削除すると、元のromfsの内容に「戻る」(whiteout未実装の既知の挙動) --- */
    ok(vfs_remove("readonly.txt") == 0, "remove the ramfs shadow copy");
    fp = vfs_fopen("readonly.txt", "r");
    ok(fp != NULL, "romfs original becomes visible again after shadow is removed");
    memset(buf, 0, sizeof(buf));
    vfs_fread(buf, 1, sizeof(buf) - 1, fp);
    ok(strcmp(buf, "from romfs\n") == 0, "content reverts to the original romfs data");
    vfs_fclose(fp);

    /* romfsだけにしか無い(一度もramfsでshadowされていない)ファイルはremoveできない */
    ok(vfs_remove("readonly.txt") == -1, "can't remove a romfs-only (never-shadowed) file");

    /* ramfsのみのファイルは普通に削除できる */
    ok(vfs_remove("new.txt") == 0, "remove a ramfs-only file");
    ok(vfs_fopen("new.txt", "r") == NULL, "removed ramfs-only file is gone");

    /* vfs_init()でramfs側だけがリセットされる (romfsは影響を受けない) */
    fp = vfs_fopen("keepme.txt", "w");
    vfs_fwrite("x", 1, 1, fp);
    vfs_fclose(fp);
    vfs_init();
    ok(vfs_fopen("keepme.txt", "r") == NULL, "vfs_init resets ramfs contents");
    fp = vfs_fopen("readonly.txt", "r");
    ok(fp != NULL, "vfs_init does not affect romfs contents");
    vfs_fclose(fp);

    /* 先頭の"./"は無いものとして扱う(@INCの"."エントリがrequireで
     * "./foo.pm"のようなパスを作るため、これを剥がさないとramfs/romfs
     * 上のファイルが「別名」扱いになって見つからない)。 */
    fp = vfs_fopen("dotslash.txt", "w");
    vfs_fwrite("hi", 1, 2, fp);
    vfs_fclose(fp);
    fp = vfs_fopen("./dotslash.txt", "r");
    ok(fp != NULL, "fopen strips a leading './' (ramfs)");
    vfs_fclose(fp);
    {
        struct vfs_stat st;
        ok(vfs_stat("./dotslash.txt", &st) == 0 && st.size == 2,
            "stat also strips a leading './' (ramfs)");
    }
    fp = vfs_fopen("./readonly.txt", "r");
    ok(fp != NULL, "fopen strips a leading './' (romfs fallback)");
    vfs_fclose(fp);
    ok(vfs_remove("./dotslash.txt") == 0, "remove also strips a leading './'");

    if (failures) {
        printf("FAILED %d\n", failures);
        return 1;
    }
    printf("ALL TESTS PASSED\n");
    return 0;
}
