/*
 * picotime.c - picotime.h の実装。
 */
#include <string.h>
#include "picotime.h"

/* 実RTCが無いため、time()は常にこの固定値を返す。実機にRTC/UARTからの
 * 時刻同期を実装する際に差し替える想定(Phase 7/8)。 */
#define PICOPERL_FIXED_TIME ((time_t)0)

time_t
picoperl_time(time_t *tp)
{
    if (tp)
        *tp = PICOPERL_FIXED_TIME;
    return PICOPERL_FIXED_TIME;
}

static int
is_leap_year(long y)
{
    return (y % 4 == 0 && (y % 100 != 0 || y % 400 == 0));
}

/* 本物のlocaltime(3)と同じく、呼び出しの都度上書きされる内部バッファを
 * 指すポインタを返す(non-reentrant契約。呼び出し元のtime64.cの
 * S_localtime_rは即座にmemcpyで自分のバッファへコピーするため問題ない)。 */
static struct tm result;

struct tm *
picoperl_localtime(const time_t *timep)
{
    long days, rem;
    long z, era, y;
    unsigned long doe, yoe, doy, mp, d, m;
    static const int cum_days[12] =
        { 0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334 };

    if (!timep)
        return NULL;

    days = (long)(*timep / 86400);
    rem = (long)(*timep % 86400);
    if (rem < 0) {
        rem += 86400;
        days -= 1;
    }

    memset(&result, 0, sizeof(result));
    result.tm_hour = (int)(rem / 3600);
    result.tm_min = (int)((rem % 3600) / 60);
    result.tm_sec = (int)(rem % 60);
    /* 1970-01-01はThursday(tm_wday==4) */
    result.tm_wday = (int)(((days % 7) + 7 + 4) % 7);

    /* civil_from_days (Howard Hinnant): daysをproleptic Gregorian暦の
     * 年/月/日に変換する。mp<10の分岐でのunsigned longの意図的な
     * アンダーフロー(-9を足す)は元アルゴリズム通りで、well-definedな
     * unsigned演算の折り返しを利用している。 */
    z = days + 719468L;
    era = (z >= 0 ? z : z - 146096L) / 146097L;
    doe = (unsigned long)(z - era * 146097L);
    yoe = (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365;
    y = (long)yoe + era * 400L;
    doy = doe - (365 * yoe + yoe / 4 - yoe / 100);
    mp = (5 * doy + 2) / 153;
    d = doy - (153 * mp + 2) / 5 + 1;
    m = mp + (mp < 10 ? 3UL : (unsigned long)-9L);
    y += (m <= 2) ? 1 : 0;

    result.tm_year = (int)(y - 1900);
    result.tm_mon = (int)(m - 1);
    result.tm_mday = (int)d;
    result.tm_yday = cum_days[result.tm_mon] + (result.tm_mday - 1) +
                      ((result.tm_mon >= 2 && is_leap_year(y)) ? 1 : 0);
    result.tm_isdst = 0;

    return &result;
}
