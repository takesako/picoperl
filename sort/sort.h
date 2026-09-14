/*
 * sort.h - libcのqsort(3)に依存しない、picoperl自前のソート実装。
 *
 * 設計方針: qsort(3)と同じシグネチャ・呼び出し規約(比較関数の符号だけを
 * 見る)を保ちつつ、中身は挿入ソート(insertion sort)にする。
 *   - 時間計算量はO(n^2)だが、実際の唯一の呼び出し元(op.cの
 *     tr///コンパイル時、Unicode範囲リストのマージ処理)ではnは
 *     ソース中の文字範囲の個数程度で常に小さく、実用上問題にならない
 *   - 再帰を一切使わない(quicksortは平均的には速いが、最悪ケースで
 *     O(n)の再帰深さになりうる。スタックが限られる組み込み環境では
 *     この再帰深さの不確実性を避け、常に一定のスタック使用量で済む
 *     非再帰アルゴリズムを優先する、というのがこのTODO項目の方針)
 *   - 要素1個分の一時バッファだけをmallocする(要素サイズ`size`は
 *     呼び出し時の可変長のためスタック上の固定バッファでは対応できない。
 *     mallocの置き換え自体は別のTODO項目)
 */
#ifndef PICOPERL_SORT_H
#define PICOPERL_SORT_H

#include <stddef.h>

void picoperl_qsort(void *base, size_t nmemb, size_t size,
                     int (*compar)(const void *, const void *));

#endif /* PICOPERL_SORT_H */
