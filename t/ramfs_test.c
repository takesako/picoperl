/*
 * ramfs.c(書き込み可能インメモリファイルシステム)の単体テスト。
 * まだPerl/PerlIOには一切繋いでおらず、ramfs.hのAPIだけをCから直接
 * たたいて新規作成・読み取り・更新・追記・削除・stat・seekを検証する。
 * ramfs.h は ../ramfs/ に置くため、romfs_test.c と同様に呼び出し側の
 * Makefile で include path (-I) を通す前提。
 */
#include <stdio.h>
#include <string.h>
#include "ramfs.h"

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
    ramfs_FILE *fp;
    char buf[64];
    struct ramfs_stat st;
    unsigned long n;

    ramfs_init();

    /* 存在しないファイルは "r" では開けない */
    fp = ramfs_fopen("no/such/file.txt", "r");
    ok(fp == NULL, "fopen r on missing file fails");

    ok(ramfs_stat("hello.txt", &st) == -1, "stat on missing file fails");

    /* 新規作成 */
    fp = ramfs_fopen("hello.txt", "w");
    ok(fp != NULL, "fopen w creates new file");
    n = ramfs_fwrite("hello", 1, 5, fp);
    ok(n == 5, "fwrite returns nmemb written");
    ramfs_fclose(fp);

    ok(ramfs_stat("hello.txt", &st) == 0 && st.size == 5, "stat after create");

    /* 読み取り */
    fp = ramfs_fopen("hello.txt", "r");
    ok(fp != NULL, "fopen r on existing file");
    memset(buf, 0, sizeof(buf));
    n = ramfs_fread(buf, 1, sizeof(buf) - 1, fp);
    ok(n == 5 && strcmp(buf, "hello") == 0, "fread returns written content");
    ok(ramfs_feof(fp), "feof set after reading past end");
    n = ramfs_fwrite("x", 1, 1, fp);
    ok(n == 0, "fwrite on read-only handle fails");
    ramfs_fclose(fp);

    /* 更新: w で開き直すと内容がtruncateされる */
    fp = ramfs_fopen("hello.txt", "w");
    ramfs_fwrite("bye", 1, 3, fp);
    ramfs_fclose(fp);
    ok(ramfs_stat("hello.txt", &st) == 0 && st.size == 3, "fopen w truncates existing file");
    fp = ramfs_fopen("hello.txt", "r");
    memset(buf, 0, sizeof(buf));
    ramfs_fread(buf, 1, sizeof(buf) - 1, fp);
    ok(strcmp(buf, "bye") == 0, "content reflects truncate+rewrite");
    ramfs_fclose(fp);

    /* r+ での部分書き換え (先頭を上書き) */
    fp = ramfs_fopen("hello.txt", "r+");
    ok(fp != NULL, "fopen r+ on existing file");
    ramfs_fwrite("BY", 1, 2, fp);
    ramfs_fclose(fp);
    fp = ramfs_fopen("hello.txt", "r");
    memset(buf, 0, sizeof(buf));
    ramfs_fread(buf, 1, sizeof(buf) - 1, fp);
    ok(strcmp(buf, "BYe") == 0, "r+ overwrites in place without truncating");
    ramfs_fclose(fp);

    /* 追記 */
    fp = ramfs_fopen("hello.txt", "a");
    ok(fp != NULL, "fopen a on existing file");
    ramfs_fwrite("!!!", 1, 3, fp);
    ramfs_fclose(fp);
    fp = ramfs_fopen("hello.txt", "r");
    memset(buf, 0, sizeof(buf));
    n = ramfs_fread(buf, 1, sizeof(buf) - 1, fp);
    ok(n == 6 && strcmp(buf, "BYe!!!") == 0, "append adds to the end");
    ramfs_fclose(fp);

    /* a は途中にseekしても常に末尾に書く */
    fp = ramfs_fopen("hello.txt", "a+");
    ramfs_fseek(fp, 0, RAMFS_SEEK_SET);
    ramfs_fwrite("Z", 1, 1, fp);
    memset(buf, 0, sizeof(buf));
    n = ramfs_fread(buf, 1, sizeof(buf) - 1, fp);
    ok(n == 0, "read position stays where seek left it after append-write");
    ramfs_fclose(fp);
    fp = ramfs_fopen("hello.txt", "r");
    memset(buf, 0, sizeof(buf));
    n = ramfs_fread(buf, 1, sizeof(buf) - 1, fp);
    ok(n == 7 && strcmp(buf, "BYe!!!Z") == 0, "a+ write always appends regardless of seek");
    ramfs_fclose(fp);

    /* seek: SET/CUR/END */
    fp = ramfs_fopen("hello.txt", "r");
    ok(ramfs_fseek(fp, 3, RAMFS_SEEK_SET) == 0 && ramfs_ftell(fp) == 3, "fseek SET");
    ok(ramfs_fseek(fp, 1, RAMFS_SEEK_CUR) == 0 && ramfs_ftell(fp) == 4, "fseek CUR");
    ok(ramfs_fseek(fp, 0, RAMFS_SEEK_END) == 0 && ramfs_ftell(fp) == 7, "fseek END");
    ok(ramfs_fseek(fp, -100, RAMFS_SEEK_SET) == -1, "fseek before start fails");
    ramfs_fclose(fp);

    /* seek で末尾を越えてから書くと、間はゼロ埋めされる (sparse write) */
    fp = ramfs_fopen("sparse.bin", "w");
    ramfs_fseek(fp, 4, RAMFS_SEEK_SET);
    ramfs_fwrite("X", 1, 1, fp);
    ramfs_fclose(fp);
    ok(ramfs_stat("sparse.bin", &st) == 0 && st.size == 5, "sparse write extends size");
    fp = ramfs_fopen("sparse.bin", "r");
    memset(buf, 0, sizeof(buf));
    ramfs_fread(buf, 1, 5, fp);
    ok(memcmp(buf, "\0\0\0\0X", 5) == 0, "gap before sparse write is zero-filled");
    ramfs_fclose(fp);

    /* 削除 */
    ok(ramfs_remove("hello.txt") == 0, "remove existing file");
    ok(ramfs_remove("hello.txt") == -1, "remove twice fails");
    ok(ramfs_fopen("hello.txt", "r") == NULL, "removed file can't be read");
    ok(ramfs_stat("hello.txt", &st) == -1, "removed file has no stat");
    /* w で作り直せば復活する(別ファイルとして新規作成) */
    fp = ramfs_fopen("hello.txt", "w");
    ok(fp != NULL, "recreate file with same name after remove");
    ramfs_fclose(fp);

    /* 名前が長すぎるファイルは拒否する */
    {
        char longname[RAMFS_NAME_MAX + 10];
        memset(longname, 'a', sizeof(longname) - 1);
        longname[sizeof(longname) - 1] = '\0';
        ok(ramfs_fopen(longname, "w") == NULL, "name too long is rejected");
    }

    /* ramfs_init() で全ファイルが消える */
    ramfs_init();
    ok(ramfs_stat("sparse.bin", &st) == -1, "ramfs_init clears all files");

    if (failures) {
        printf("FAILED %d\n", failures);
        return 1;
    }
    printf("ALL TESTS PASSED\n");
    return 0;
}
