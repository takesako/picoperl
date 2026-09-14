/*
 * env.c - env.h の実装。
 */
#include <stdlib.h>
#include <string.h>
#include "env.h"

struct env_node {
    char *entry;            /* "NAME=VALUE"、所有権を持つ、freeしない */
    struct env_node *next;
};

static struct env_node *env_head = NULL;

static size_t
name_len(const char *entry)
{
    const char *eq = strchr(entry, '=');
    return eq ? (size_t)(eq - entry) : strlen(entry);
}

static struct env_node *
find_node(const char *name, size_t namelen)
{
    struct env_node *p;

    for (p = env_head; p; p = p->next) {
        if (name_len(p->entry) == namelen && strncmp(p->entry, name, namelen) == 0)
            return p;
    }
    return NULL;
}

void
env_reset(void)
{
    /* エントリ自体は元々freeしない設計のため、リストのノードだけを
     * 解放する(entry文字列はputenvの契約上呼び出し側の所有物のまま)。 */
    struct env_node *p, *next;

    for (p = env_head; p; p = next) {
        next = p->next;
        free(p);
    }
    env_head = NULL;
}

const char *
picoperl_getenv(const char *name)
{
    struct env_node *n;
    const char *eq;

    if (!name)
        return NULL;
    n = find_node(name, strlen(name));
    if (!n)
        return NULL;
    eq = strchr(n->entry, '=');
    return eq ? eq + 1 : "";
}

int
picoperl_putenv(char *string)
{
    size_t namelen;
    struct env_node *n;

    if (!string || !strchr(string, '='))
        return -1;

    namelen = name_len(string);
    n = find_node(string, namelen);
    if (n) {
        n->entry = string; /* 古いポインタはfreeしない(putenv(3)と同じ割り切り) */
        return 0;
    }

    n = (struct env_node *)malloc(sizeof *n);
    if (!n)
        return -1;
    n->entry = string;
    n->next = env_head;
    env_head = n;
    return 0;
}

void
env_init(char **envp)
{
    char **e;

    if (!envp)
        return;
    for (e = envp; *e; e++) {
        size_t len = strlen(*e);
        char *copy;

        if (!strchr(*e, '='))
            continue; /* '='の無いエントリは無視する(putenvの契約上不正) */

        /* envp自体はOSがプロセス起動時に用意した領域で、いつ無効に
         * なるか分からない(execve後の再利用等)ため複製して所有権を
         * 持つ(picoperl_putenvの「以後freeしない」契約に合わせる)。
         */
        copy = (char *)malloc(len + 1);
        if (!copy)
            continue;
        memcpy(copy, *e, len + 1);
        picoperl_putenv(copy);
    }
}
