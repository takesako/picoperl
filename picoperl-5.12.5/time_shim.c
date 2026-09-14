/*
 * time_shim.c - plain picoperl向けの既定実装。
 *
 * libc/time.h は time/localtime を picoperl_time()/
 * picoperl_localtime() にリダイレクトする関数マクロを定義している。
 * この既定実装は「常に本物のlibc time(3)/localtime(3)へそのまま
 * 委譲するだけ」のパススルーで、plain picoperlの挙動は今までと
 * 一切変わらない。
 *
 * __attribute__((weak))にしているのは、romperl(../libc/picotime.c)が
 * 同名の強いシンボルでリンク時に上書きし、libcに依存しない自前実装
 * (固定エポックのtime() + ゼロから計算するlocaltime())に差し替え
 * られるようにするため。romperlはpicoperl-5.12.5の.oをコピーせず
 * 参照するだけなので(このファイルも例外ではない)、コンパイル時の
 * #ifdefでpicoperl/romperlを出し分けることができず、pp_requireの
 * romperl_find_for_compileと同じ「弱いデフォルト+強い上書き」の
 * リンク時解決に頼っている(env_shim.c/posix_shim.c/stdio_shim.c/
 * sort_shim.c/rand_shim.cと同じ仕組み)。
 *
 * 呼び出し先を`(time)(...)`のように余分な括弧で囲っているのは、
 * libc/time.hが同名を関数マクロに置き換えているため、素の
 * `time(...)`と書くと自分自身(picoperl_time)を再帰呼び出しして
 * しまうのを防ぐため。
 */
#include <time.h>

__attribute__((weak)) time_t
picoperl_time(time_t *tp)
{
    return (time)(tp);
}

__attribute__((weak)) struct tm *
picoperl_localtime(const time_t *timep)
{
    return (localtime)(timep);
}
