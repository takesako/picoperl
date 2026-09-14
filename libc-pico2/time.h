/*
 * picoperl向けlibcシム: <time.h>
 *
 * time()/localtime()をpicoperl自前の実装(romperl/time/picotime.c)経由の
 * picoperl_time()/picoperl_localtime()にリダイレクトする。
 *
 * gmtime()は対象外: pp_sys.cに#includeされるtime64.cの
 * S_gmtime64_r()が既にlibcに依存しない純粋なC実装のカレンダー変換を
 * 行っており(`SHOULD_USE_SYSTEM_GMTIME`がこのビルドでは常に偽になる
 * ため、実際に本物のgmtime()を呼ぶ経路は最適化で消える。
 * `nm -u`でgmtimeがリンクされていないことで確認済み)、リダイレクト
 * する必要が無い。一方localtime()は、S_localtime64_r()が
 * 「safe yearにマップした時刻をタイムゾーン変換する」ため最終段で
 * 必ず呼ぶので、こちらは実際にリダイレクトが必要。
 *
 * libc依存を削るのはromperl側だけでよく、plain picoperlはNV=floatの
 * 最小実装のままにする方針のため(TODO.md「Phase 5」参照)、他の
 * libc-pico2シムと同じ「弱いデフォルト実装(project rootの
 * time_shim.c、本物のlibc関数へのパススルー)+ romperl側の強い実装
 * (リンク時にpp_requireのromperl_find_for_compileと同じ弱い/強い
 * シンボルの仕組みで上書き)」という型を使う。
 */
#ifndef PICOPERL_LIBC_PICO2_TIME_H
#define PICOPERL_LIBC_PICO2_TIME_H

#include_next <time.h>

extern time_t picoperl_time(time_t *tp);
extern struct tm *picoperl_localtime(const time_t *timep);

#undef  time
#define time(tp)          picoperl_time((tp))
#undef  localtime
#define localtime(timep)  picoperl_localtime((timep))

#endif /* PICOPERL_LIBC_PICO2_TIME_H */
