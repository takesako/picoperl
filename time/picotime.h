/*
 * picotime.h - libcのtime(3)/localtime(3)に依存しない、picoperl自前の
 * 実装。ファイル名を素直に"time.h"にすると、-I.(このディレクトリ)を
 * include pathに含むビルドで`#include <time.h>`(本物のシステム
 * ヘッダ)がこのファイル自身を再帰的にincludeしてしまう(自己衝突)。
 * それを避けるためあえて"picotime.h"という名前にしている。
 *
 * 実RTCが無い前提のため、time()は常に固定のエポック値を返す
 * (「固定エポックの時刻を返す」というTODOの方針通り)。
 *
 * localtime()は、time64.c(pp_sys.cに#includeされ、picoperlのgmtime/
 * localtime組み込み関数の中身を担う)が最終段で必ず呼ぶ関数で、
 * "safe year"にマップした時刻をタイムゾーン変換するために使われる
 * (time64.c自身のカレンダー変換(S_gmtime64_r)は既にlibcに依存しない
 * 純粋なC実装だが、localtime相当の「ローカルタイムゾーンへの変換」
 * だけは最終的にlibcのlocaltime()に委ねている)。RP2350にはタイムゾーン
 * データベースが無いため、ローカルタイム=UTC(オフセット0)として扱う
 * のが妥当という判断で、picoperl_localtime()はUTC(=gmtime相当)の
 * カレンダー変換をゼロから計算する。
 *
 * アルゴリズムはHoward Hinnantのproleptic Gregorian暦アルゴリズム
 * (civil_from_days、http://howardhinnant.github.io/date_algorithms.html
 * で公開されている、正しさが広く検証された定数時間アルゴリズム)。
 */
#ifndef PICOPERL_PICOTIME_H
#define PICOPERL_PICOTIME_H

#include <time.h>

time_t picoperl_time(time_t *tp);
struct tm *picoperl_localtime(const time_t *timep);

#endif /* PICOPERL_PICOTIME_H */
