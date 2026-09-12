/*
 * picoperl向けlibcシム。libc-pico2/unistd.h も参照。
 * wait/waitpid は子プロセスの存在を前提とするため、fork() が常に失敗する
 * この環境では呼ばれても意味がない。常に「待つべき子がいない」エラーを返す。
 */
#ifndef PICOPERL_LIBC_PICO2_SYS_WAIT_H
#define PICOPERL_LIBC_PICO2_SYS_WAIT_H

#include_next <sys/wait.h>
#include <errno.h>

#undef  wait
#define wait(statusp)               (errno = ECHILD, (pid_t)-1)
#undef  waitpid
#define waitpid(pid, statusp, opt)  (errno = ECHILD, (pid_t)-1)

#endif /* PICOPERL_LIBC_PICO2_SYS_WAIT_H */
