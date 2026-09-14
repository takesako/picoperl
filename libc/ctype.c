/*
 * ctype.c - picoctype.h の実装。
 *
 * 単純なASCII範囲の比較のみ("C"ロケール相当)。テーブルは使わない
 * (このビルドはsetlocale()を一度も呼ばず常に"C"ロケールのままなので、
 * テーブルを持つ意味が無い。シンプルさ優先)。
 */
#include "picoctype.h"

int
picoperl_isupper(int c)
{
    return c >= 'A' && c <= 'Z';
}

int
picoperl_islower(int c)
{
    return c >= 'a' && c <= 'z';
}

int
picoperl_isalpha(int c)
{
    return picoperl_isupper(c) || picoperl_islower(c);
}

int
picoperl_isdigit(int c)
{
    return c >= '0' && c <= '9';
}

int
picoperl_isalnum(int c)
{
    return picoperl_isalpha(c) || picoperl_isdigit(c);
}

int
picoperl_isspace(int c)
{
    return c == ' ' || c == '\t' || c == '\n' || c == '\v' || c == '\f' || c == '\r';
}

int
picoperl_iscntrl(int c)
{
    return (c >= 0 && c < 0x20) || c == 0x7f;
}

int
picoperl_isprint(int c)
{
    return c >= 0x20 && c < 0x7f;
}

int
picoperl_isgraph(int c)
{
    return c > 0x20 && c < 0x7f;
}

int
picoperl_ispunct(int c)
{
    return picoperl_isprint(c) && !picoperl_isalnum(c) && c != ' ';
}

int
picoperl_isxdigit(int c)
{
    return picoperl_isdigit(c) || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
}

int
picoperl_isascii(int c)
{
    return (unsigned int)c <= 0x7f;
}

int
picoperl_isblank(int c)
{
    return c == ' ' || c == '\t';
}

int
picoperl_toupper(int c)
{
    return picoperl_islower(c) ? c - 'a' + 'A' : c;
}

int
picoperl_tolower(int c)
{
    return picoperl_isupper(c) ? c - 'A' + 'a' : c;
}
