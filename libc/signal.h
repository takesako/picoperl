/*
 * picoperl向けlibcシム。libc/unistd.h も参照。
 * kill/killpg はプロセス間シグナル送信を前提とするため、OSなしターゲットでは
 * 常に失敗させる。signal()/raise() 等、自プロセス内で完結する機能は
 * #include_next した本物の宣言をそのまま使う。
 */
#ifndef PICOPERL_LIBC_SIGNAL_H
#define PICOPERL_LIBC_SIGNAL_H

#include_next <signal.h>
#include <errno.h>

#undef  kill
#define kill(pid, sig)      (errno = ENOSYS, -1)
#undef  killpg
#define killpg(pgrp, sig)   (errno = ENOSYS, -1)

#endif /* PICOPERL_LIBC_SIGNAL_H */
