/*
 * time.c(libcのtime/localtimeに依存しない実装)の単体テスト。
 *
 * picoperl_localtime()はUTC(gmtime相当)のカレンダー変換をゼロから
 * 計算する設計のため、その正しさは本物のlibc gmtime(3)の結果と
 * 突き合わせて検証する(x86_64ホストの実行環境では実装済みで
 * 信頼できる基準として使える)。
 */
#include <stdio.h>
#include <time.h>
#include "picotime.h"

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

static int
tm_equal(const struct tm *a, const struct tm *b)
{
    return a->tm_sec == b->tm_sec && a->tm_min == b->tm_min &&
           a->tm_hour == b->tm_hour && a->tm_mday == b->tm_mday &&
           a->tm_mon == b->tm_mon && a->tm_year == b->tm_year &&
           a->tm_wday == b->tm_wday && a->tm_yday == b->tm_yday;
}

static void
check_against_gmtime(time_t t, const char *label)
{
    struct tm *ours = picoperl_localtime(&t);
    struct tm real;
    struct tm *realp = gmtime_r(&t, &real);
    char msg[128];

    snprintf(msg, sizeof(msg), "picoperl_localtime(%s) matches real gmtime()", label);
    ok(ours != NULL && realp != NULL && tm_equal(ours, realp), msg);
}

int
main(void)
{
    time_t t;

    t = 0;
    ok(picoperl_time(&t) == 0 && t == 0, "picoperl_time returns the fixed epoch (0)");
    ok(picoperl_time(NULL) == 0, "picoperl_time(NULL) also returns the fixed epoch");

    check_against_gmtime(0, "epoch 0 / 1970-01-01");
    check_against_gmtime(1, "epoch 1");
    check_against_gmtime(86399, "23:59:59 on day 0");
    check_against_gmtime(86400, "1970-01-02");
    check_against_gmtime(946684800, "2000-01-01 (century leap year)");
    check_against_gmtime(1709164800, "2024-02-29 (ordinary leap year)");
    check_against_gmtime(1700000000, "an arbitrary recent timestamp");
    check_against_gmtime(2147483647, "32-bit time_t max (2038 problem boundary)");
    check_against_gmtime(-1, "one second before the epoch");
    check_against_gmtime(-86400, "1969-12-31");
    check_against_gmtime(-2208988800, "1900-01-01");

    ok(picoperl_localtime(NULL) == NULL, "picoperl_localtime(NULL) returns NULL");

    if (failures) {
        printf("FAILED %d\n", failures);
        return 1;
    }
    printf("ALL TESTS PASSED\n");
    return 0;
}
