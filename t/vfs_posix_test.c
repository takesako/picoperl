/*
 * vfs_posix.c(vfs.hをstat/unlinkとして使えるようにするラッパー)の
 * 単体テスト。romperlが実際にリンクする強い実装(picoperl_stat/
 * picoperl_unlink)をvfs/vfs_posix.hのシグネチャのまま直接呼び、
 * romfsフォールバック・ramfs優先・削除の可否をCから確認する。
 * ビルドはvfs/Makefileのtest-posixターゲット(vfs_test同様、
 * vfs/testdata/readonly.txtを埋め込んだromfsイメージを使う)。
 */
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include "vfs.h"
#include "vfs_posix.h"
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
    struct stat st;
    vfs_FILE *fp;

    ok(romfs_init(vfs_test_romfs_image, vfs_test_romfs_image_size), "romfs_init");
    vfs_init();

    /* picoperl_stat: romfsフォールバック */
    ok(picoperl_stat("readonly.txt", &st) == 0, "picoperl_stat finds romfs-only file");
    ok((unsigned long)st.st_size == strlen("from romfs\n"), "picoperl_stat reports romfs size");
    ok(S_ISREG(st.st_mode), "picoperl_stat reports a regular file mode");
    ok(picoperl_stat("nowhere.txt", &st) == -1, "picoperl_stat fails for unknown name");

    /* picoperl_unlink: romfs専用ファイルは削除できない */
    ok(picoperl_unlink("readonly.txt") == -1, "picoperl_unlink refuses a romfs-only file");
    ok(picoperl_stat("readonly.txt", &st) == 0, "romfs-only file survives the failed unlink");

    /* ramfs側に(vfs_fopen経由で直接)書いたファイルはstat/unlinkできる。
     * まだPerl側からvfs経由で書き込む手段(open()の統合)は無いため、
     * ここではvfs_fopenを直接使ってramfsにファイルを作る。 */
    fp = vfs_fopen("made.txt", "w");
    ok(fp != NULL, "vfs_fopen creates a ramfs file directly (test setup)");
    vfs_fwrite("hi", 1, 2, fp);
    vfs_fclose(fp);

    ok(picoperl_stat("made.txt", &st) == 0 && (unsigned long)st.st_size == 2,
        "picoperl_stat sees a ramfs-only file");
    ok(picoperl_unlink("made.txt") == 0, "picoperl_unlink removes a ramfs file");
    ok(picoperl_stat("made.txt", &st) == -1, "removed ramfs file is gone from stat");

    if (failures) {
        printf("FAILED %d\n", failures);
        return 1;
    }
    printf("ALL TESTS PASSED\n");
    return 0;
}
