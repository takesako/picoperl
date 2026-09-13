/*
 * pp_require を直接ROMFSに繋ぐための関数。picoperl-5.12.5/pp_ctl.c
 * (make-picoperl.shがパッチする)から、パッチで追加した弱シンボルの
 * デフォルト実装を、ここで定義する本物の(強い)実装がリンク時に
 * 上書きする。
 *
 * picoperlはこの.oをリンクしないので、picoperl側はpp_ctl.cに埋め込まれた
 * 「常にNULLを返す」弱いstubのまま(挙動は今まで通り、ROMFSは一切見ない)。
 * romperlはこの.oもリンクするので、強いシンボルが優先されて実際に
 * ROMFSを検索するようになる。
 *
 * perl.h / XSUB.h は使わないプレーンなCなので、romfs.cと同じくOPTIMIZEのみで
 * コンパイルできる。
 */
#include <stdio.h>
#include "romfs.h"

const void *
romperl_find_for_compile(const char *name, unsigned long *out_len)
{
    char path[ROMFS_NAME_MAX];
    int n;

    /* Romperl::Boot の@INCフック ("lib/$filename") と同じ命名規則 */
    n = snprintf(path, sizeof(path), "lib/%s", name);
    if (n < 0 || (size_t)n >= sizeof(path))
        return (const void *)0;

    return romfs_data_for_compile(path, out_len);
}
