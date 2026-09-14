/*
 * env.c(libcのgetenv/putenvに依存しない環境変数ストア)の単体テスト。
 * まだPerlには一切繋いでおらず、env.hのAPIだけをCから直接たたく。
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "env.h"

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

static char *
dupstr(const char *s)
{
    char *p = (char *)malloc(strlen(s) + 1);
    strcpy(p, s);
    return p;
}

int
main(void)
{
    env_reset();

    ok(picoperl_getenv("NOPE") == NULL, "getenv on unset name returns NULL");

    ok(picoperl_putenv(dupstr("FOO=bar")) == 0, "putenv FOO=bar succeeds");
    ok(strcmp(picoperl_getenv("FOO"), "bar") == 0, "getenv FOO returns bar");

    /* 前方一致で誤検出しないこと("FOO" vs "FOOBAR") */
    ok(picoperl_getenv("FOOBAR") == NULL, "getenv doesn't confuse FOO with FOOBAR");
    ok(picoperl_putenv(dupstr("FOOBAR=baz")) == 0, "putenv FOOBAR=baz succeeds");
    ok(strcmp(picoperl_getenv("FOO"), "bar") == 0, "FOO still bar after adding FOOBAR");
    ok(strcmp(picoperl_getenv("FOOBAR"), "baz") == 0, "FOOBAR returns baz");

    /* 上書き */
    ok(picoperl_putenv(dupstr("FOO=updated")) == 0, "putenv can overwrite an existing name");
    ok(strcmp(picoperl_getenv("FOO"), "updated") == 0, "getenv sees the updated value");

    /* '='の無い文字列は失敗する */
    ok(picoperl_putenv(dupstr("NOEQUALS")) == -1, "putenv without '=' fails");

    /* 空の値("NAME=") */
    ok(picoperl_putenv(dupstr("EMPTY=")) == 0, "putenv NAME= (empty value) succeeds");
    ok(picoperl_getenv("EMPTY") != NULL && strlen(picoperl_getenv("EMPTY")) == 0,
        "getenv returns an empty (non-NULL) string for NAME=");

    /* env_reset()で全消去 */
    env_reset();
    ok(picoperl_getenv("FOO") == NULL, "env_reset clears everything");

    /* env_init(envp)によるシード */
    {
        static char e1[] = "A=1";
        static char e2[] = "B=2";
        static char *fake_envp[] = { e1, e2, NULL };
        env_init(fake_envp);
        ok(strcmp(picoperl_getenv("A"), "1") == 0, "env_init seeds from envp (A)");
        ok(strcmp(picoperl_getenv("B"), "2") == 0, "env_init seeds from envp (B)");
    }

    env_reset();
    ok(picoperl_getenv("A") == NULL, "env_reset clears env_init-seeded entries too");

    if (failures) {
        printf("FAILED %d\n", failures);
        return 1;
    }
    printf("ALL TESTS PASSED\n");
    return 0;
}
