/*
 * picoctype.h - libcの<ctype.h>(is*系/to*系マクロが内部で呼ぶ
 * __ctype_b_loc(3)等のlocale依存テーブル)に依存しない、picoperl自前の
 * 文字分類/変換実装。
 *
 * ファイル名を素直に"ctype.h"にすると、romperlのビルド(-I../libc)で
 * `#include <ctype.h>`(本物のシステムヘッダのつもり)が、シム自身の
 * "ctype.h"(このファイルとは別に用意してある)と衝突する
 * (picotime.h/time.hで踏んだのと同じ罠)。シム側は"ctype.h"のままで
 * 良い(#include_nextで本物を辿れるため)が、こちらの実装用ヘッダは
 * 名前を変えて衝突を避けている。
 *
 * このビルドはsetlocale()を一度も呼ばない(nm -uで未リンクを確認済み)
 * ため、実行中ずっと"C"(デフォルト)ロケールのまま変わらない。つまり
 * 現状のlibc版is*系/to*系も実質ASCII固定の分類しかしていない。ここでの
 * 自前実装はそれと同じ挙動をASCII範囲のみの単純な比較で再現するだけで、
 * 動作を変えずにlibcのlocale依存テーブル(__ctype_b_loc)への依存を断つ。
 *
 * handy.h のisALPHA_LC/isSPACE_LC/isDIGIT_LC/toUPPER_LC等
 * (use locale時やPOSIX文字クラス[[:alpha:]]等で使われる、localeを
 * 考慮する版の分類マクロ)が最終的にこれらを呼ぶ。
 */
#ifndef PICOPERL_PICOCTYPE_H
#define PICOPERL_PICOCTYPE_H

int picoperl_isalpha(int c);
int picoperl_isdigit(int c);
int picoperl_isspace(int c);
int picoperl_isupper(int c);
int picoperl_islower(int c);
int picoperl_isalnum(int c);
int picoperl_ispunct(int c);
int picoperl_iscntrl(int c);
int picoperl_isprint(int c);
int picoperl_isgraph(int c);
int picoperl_isxdigit(int c);
int picoperl_isascii(int c);
int picoperl_isblank(int c);
int picoperl_toupper(int c);
int picoperl_tolower(int c);

#endif /* PICOPERL_PICOCTYPE_H */
