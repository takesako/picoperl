/*
 * ctype.c(libcのlocale依存is*系/to*系に依存しない、ASCII範囲の単純な
 * 比較による実装)の単体テスト。本物のlibc <ctype.h> の結果("C"ロケール
 * 固定、このビルドはsetlocale()を一度も呼ばないため常にこれと同じ)と
 * 0-255の全バイト値で突き合わせて検証する。
 */
#include <stdio.h>
#include <ctype.h>
#include "picoctype.h"

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
    int c;
    int alpha_ok = 1, digit_ok = 1, space_ok = 1, upper_ok = 1, lower_ok = 1;
    int alnum_ok = 1, punct_ok = 1, cntrl_ok = 1, print_ok = 1, graph_ok = 1;
    int xdigit_ok = 1, ascii_ok = 1, blank_ok = 1, toupper_ok = 1, tolower_ok = 1;

    for (c = 0; c <= 255; c++) {
        if (!!picoperl_isalpha(c) != !!isalpha(c)) alpha_ok = 0;
        if (!!picoperl_isdigit(c) != !!isdigit(c)) digit_ok = 0;
        if (!!picoperl_isspace(c) != !!isspace(c)) space_ok = 0;
        if (!!picoperl_isupper(c) != !!isupper(c)) upper_ok = 0;
        if (!!picoperl_islower(c) != !!islower(c)) lower_ok = 0;
        if (!!picoperl_isalnum(c) != !!isalnum(c)) alnum_ok = 0;
        if (!!picoperl_ispunct(c) != !!ispunct(c)) punct_ok = 0;
        if (!!picoperl_iscntrl(c) != !!iscntrl(c)) cntrl_ok = 0;
        if (!!picoperl_isprint(c) != !!isprint(c)) print_ok = 0;
        if (!!picoperl_isgraph(c) != !!isgraph(c)) graph_ok = 0;
        if (!!picoperl_isxdigit(c) != !!isxdigit(c)) xdigit_ok = 0;
        if (!!picoperl_isascii(c) != !!isascii(c)) ascii_ok = 0;
        if (!!picoperl_isblank(c) != !!isblank(c)) blank_ok = 0;
        if (picoperl_toupper(c) != toupper(c)) toupper_ok = 0;
        if (picoperl_tolower(c) != tolower(c)) tolower_ok = 0;
    }

    ok(alpha_ok, "isalpha matches libc for all of 0-255");
    ok(digit_ok, "isdigit matches libc for all of 0-255");
    ok(space_ok, "isspace matches libc for all of 0-255");
    ok(upper_ok, "isupper matches libc for all of 0-255");
    ok(lower_ok, "islower matches libc for all of 0-255");
    ok(alnum_ok, "isalnum matches libc for all of 0-255");
    ok(punct_ok, "ispunct matches libc for all of 0-255");
    ok(cntrl_ok, "iscntrl matches libc for all of 0-255");
    ok(print_ok, "isprint matches libc for all of 0-255");
    ok(graph_ok, "isgraph matches libc for all of 0-255");
    ok(xdigit_ok, "isxdigit matches libc for all of 0-255");
    ok(ascii_ok, "isascii matches libc for all of 0-255");
    ok(blank_ok, "isblank matches libc for all of 0-255");
    ok(toupper_ok, "toupper matches libc for all of 0-255");
    ok(tolower_ok, "tolower matches libc for all of 0-255");

    /* EOF(-1)を渡しても壊れないこと(標準ctype関数の契約) */
    ok(!picoperl_isalpha(-1) && !picoperl_isdigit(-1), "EOF (-1) doesn't crash and classifies as false");

    if (failures) {
        printf("FAILED %d\n", failures);
        return 1;
    }
    printf("ALL TESTS PASSED\n");
    return 0;
}
