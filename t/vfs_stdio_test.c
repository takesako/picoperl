/*
 * vfs_stdio.c(vfs.hをlibcのfopen系APIとして使えるようにするラッパー)の
 * 単体テスト。romperlが実際にリンクする強い実装(picoperl_fopen等)を
 * vfs/vfs_stdio.hのシグネチャのまま直接呼び、romfsフォールバック・
 * ramfsへの新規作成/更新・本物のFILE*(stdin等)への委譲をCから確認する。
 * ビルドはvfs/Makefileのtest-stdioターゲット(vfs_test同様、
 * vfs/testdata/readonly.txtを埋め込んだromfsイメージを使う)。
 */
#include <stdio.h>
#include <string.h>
#include "vfs.h"
#include "vfs_stdio.h"
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
    FILE *fp;
    char buf[64];

    ok(romfs_init(vfs_test_romfs_image, vfs_test_romfs_image_size), "romfs_init");
    vfs_init();

    /* romfsフォールバックでの読み取り */
    fp = picoperl_fopen("readonly.txt", "r");
    ok(fp != NULL, "picoperl_fopen falls back to romfs");
    memset(buf, 0, sizeof(buf));
    ok(picoperl_fread(buf, 1, sizeof(buf) - 1, fp) == strlen("from romfs\n"),
        "picoperl_fread reads the romfs content");
    ok(strcmp(buf, "from romfs\n") == 0, "content matches");
    /* buf(64byte)よりファイルが短い(11byte)ため、要求量に届かなかった
     * 時点でこの1回のfread呼び出し内で既にEOFに達している(realのfread/
     * feofと同じ挙動: 要求より少ない量しか返らなければEOF)。 */
    ok(picoperl_feof(fp) != 0, "feof set once a short read already hit EOF");
    ok(picoperl_fread(buf, 1, 1, fp) == 0, "further reads return 0 at EOF");
    ok(picoperl_fwrite("x", 1, 1, fp) == 0, "write on a read-only handle is rejected");
    ok(picoperl_fclose(fp) == 0, "fclose ok");

    /* ramfsへの新規作成・更新 */
    fp = picoperl_fopen("made.txt", "w");
    ok(fp != NULL, "picoperl_fopen w creates a ramfs-only file");
    ok(picoperl_fwrite("hello", 1, 5, fp) == 5, "fwrite returns nmemb written");
    ok(picoperl_fclose(fp) == 0, "fclose ok");

    fp = picoperl_fopen("made.txt", "r");
    memset(buf, 0, sizeof(buf));
    ok(picoperl_fread(buf, 1, sizeof(buf) - 1, fp) == 5 && strcmp(buf, "hello") == 0,
        "made.txt reads back what was written");
    picoperl_fclose(fp);

    /* fseek/ftell */
    fp = picoperl_fopen("made.txt", "r");
    ok(picoperl_fseek(fp, 2, SEEK_SET) == 0 && picoperl_ftell(fp) == 2, "fseek/ftell work");
    picoperl_fclose(fp);

    /* fgetc/fputs */
    fp = picoperl_fopen("gp.txt", "w");
    ok(picoperl_fputs("ab", fp) == 0, "fputs succeeds");
    picoperl_fclose(fp);
    fp = picoperl_fopen("gp.txt", "r");
    ok(picoperl_fgetc(fp) == 'a', "fgetc reads first byte");
    ok(picoperl_fgetc(fp) == 'b', "fgetc reads second byte");
    ok(picoperl_fgetc(fp) == EOF, "fgetc returns EOF at end");
    picoperl_fclose(fp);

    /* fprintf */
    fp = picoperl_fopen("pf.txt", "w");
    ok(picoperl_fprintf(fp, "n=%d s=%s", 42, "hi") > 0, "fprintf returns positive count");
    picoperl_fclose(fp);
    fp = picoperl_fopen("pf.txt", "r");
    memset(buf, 0, sizeof(buf));
    picoperl_fread(buf, 1, sizeof(buf) - 1, fp);
    ok(strcmp(buf, "n=42 s=hi") == 0, "fprintf output matches");
    picoperl_fclose(fp);

    /* fileno: vfsバックエンドには本物のfdが無い */
    fp = picoperl_fopen("made.txt", "r");
    ok(picoperl_fileno(fp) == -1, "fileno returns -1 for a vfs-backed handle");
    picoperl_fclose(fp);

    /* 本物のFILE*(stdin)への委譲: vfsに関係無くそのまま動くこと */
    ok(picoperl_ferror(stdin) == 0, "picoperl_ferror passes through for a real FILE*");
    ok(picoperl_fileno(stdin) == 0, "picoperl_fileno passes through and returns real fd 0 for stdin");

    if (failures) {
        printf("FAILED %d\n", failures);
        return 1;
    }
    printf("ALL TESTS PASSED\n");
    return 0;
}
