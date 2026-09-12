/*
 * romfs.c の open/read/seek/close/stat を、Perl統合前に単体で検証するための
 * スタンドアロンテスト。mkromfs.pl --carray で埋め込んだ romfs_image[] を
 * そのまま使う。
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "romfs.h"

extern const unsigned char romfs_image[];
extern const unsigned long romfs_image_size;

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
    struct romfs_stat st;
    char buf[256];
    int fd;
    long n;

    ok(romfs_init(romfs_image, romfs_image_size), "romfs_init");

    ok(romfs_stat("hello.pl", &st) == 0, "stat hello.pl found");
    ok(st.size == strlen("print \"hello from romfs\\n\";\n"), "stat hello.pl size");
    ok(romfs_stat("no/such/file", &st) == -1, "stat missing file fails");

    fd = romfs_open("hello.pl");
    ok(fd >= 0, "open hello.pl");
    memset(buf, 0, sizeof(buf));
    n = romfs_read(fd, buf, sizeof(buf) - 1);
    ok(n == (long)st.size, "read returns full size");
    ok(strcmp(buf, "print \"hello from romfs\\n\";\n") == 0, "read content matches");
    n = romfs_read(fd, buf, sizeof(buf));
    ok(n == 0, "read at EOF returns 0");
    romfs_close(fd);

    fd = romfs_open("lib/feature.pm");
    ok(fd >= 0, "open lib/feature.pm");
    ok(romfs_lseek(fd, 8, ROMFS_SEEK_SET) == 8, "lseek SET");
    memset(buf, 0, sizeof(buf));
    n = romfs_read(fd, buf, 7);
    ok(n == 7 && strncmp(buf, "feature", 7) == 0, "read after seek");
    ok(romfs_lseek(fd, 0, ROMFS_SEEK_CUR) == 15, "lseek CUR reports position");
    ok(romfs_lseek(fd, -100000, ROMFS_SEEK_SET) == 0, "lseek clamps below 0");
    romfs_close(fd);

    ok(romfs_open("does/not/exist") == -1, "open missing file fails");

    if (failures) {
        printf("FAILED %d\n", failures);
        return 1;
    }
    printf("ALL TESTS PASSED\n");
    return 0;
}
