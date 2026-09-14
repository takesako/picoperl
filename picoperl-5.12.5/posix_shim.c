/*
 * posix_shim.c - plain picoperl向けの既定実装。
 *
 * libc/sys/stat.h・unistd.h は stat/unlink を picoperl_stat()等に
 * リダイレクトする関数マクロを定義している。この既定実装は「常に本物の
 * システムコールへそのまま委譲するだけ」のパススルーで、plain picoperl
 * の挙動は今までと一切変わらない。
 *
 * open/close/read/write/lseek/fstatは意図的に対象外(TODO.md「Phase 5」
 * 参照: useperlio=undefのこのビルドでは、sysopen等がopen()で得たfdを
 * 直後に本物のfdopen()でFILE*化するため、open()だけをvfsにリダイレクト
 * するとfdopen()がEBADFで失敗する。fstatもvfs経由のfdを作る手段が無いと
 * 意味が無い。stdio全体を合わせて対応するまで保留)。
 *
 * __attribute__((weak))にしているのは、romperl(../vfs/vfs_posix.c)が
 * 同名の強いシンボルでリンク時に上書きし、ROMFS/RAMFS(vfs)経由の実装に
 * 差し替えられるようにするため。romperlはpicoperl-5.12.5の.oをコピー
 * せず参照するだけなので(このファイルも例外ではない)、コンパイル時の
 * #ifdefでpicoperl/romperlを出し分けることができず、pp_requireの
 * romperl_find_for_compileと同じ「弱いデフォルト+強い上書き」の
 * リンク時解決に頼っている。
 *
 * 呼び出し先を`(stat)(...)`のように余分な括弧で囲っているのは、
 * libc/*.hが同名を関数マクロに置き換えているため、素の
 * `stat(...)`と書くと自分自身(picoperl_stat)を再帰呼び出ししてしまう
 * のを防ぐため(関数マクロは「識別子の直後に'('」が展開条件のため、
 * `(stat)`のように直後が'('でなければ展開されず本物のシンボルを指す)。
 */
#include <unistd.h>
#include <sys/stat.h>

__attribute__((weak)) int
picoperl_stat(const char *path, struct stat *st)
{
    return (stat)(path, st);
}

__attribute__((weak)) int
picoperl_unlink(const char *path)
{
    return (unlink)(path);
}
