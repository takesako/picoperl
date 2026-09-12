/*
 * picoperl向けlibcシム。
 *
 * RP2350 (OSなしのCortex-M33) をターゲットにするにあたり、プロセス/ユーザー
 * 概念を前提とするlibc関数(fork/exec/pipe/sleep/getuid等)を「常に失敗する、
 * または固定値を返す」実装に差し替える。
 *
 * x86_64ホストではmake-picoperl.shが `-Ilibc-pico2` を最初のincludeパスに
 * 追加しているため、`#include <unistd.h>` はまずこのファイルを見つける。
 * ここでは #include_next で本物の <unistd.h> (close/read/write/lseek/
 * isatty/dup等、まだ必要な宣言) を読み込んだ上で、対象の関数だけを
 * 関数マクロで上書きする。
 */
#ifndef PICOPERL_LIBC_PICO2_UNISTD_H
#define PICOPERL_LIBC_PICO2_UNISTD_H

#include_next <unistd.h>
#include <errno.h>

/*
 * 引数無しの関数は可変引数マクロにしている。perl.h が
 * `Uid_t getuid (void);` のような素の再宣言を持っているため、通常の0引数
 * マクロ (`#define getuid() ...`) だと「(void)」を1引数として渡した扱いに
 * なりコンパイルエラーになる。`(...)` ならどちらの形でも受理できる。
 */
#undef  fork
#define fork(...)       (errno = ENOSYS, (pid_t)-1)

#undef  execl
#define execl(...)      (errno = ENOSYS, -1)
#undef  execv
#define execv(path, argv)   (errno = ENOSYS, -1)
#undef  execvp
#define execvp(file, argv)  (errno = ENOSYS, -1)

#undef  pipe
#define pipe(fds)       (errno = ENOSYS, -1)

#undef  sleep
#define sleep(seconds)  ((unsigned int)0)

#undef  getpid
#define getpid(...)     ((pid_t)1)
#undef  getuid
#define getuid(...)     ((uid_t)0)
#undef  geteuid
#define geteuid(...)    ((uid_t)0)
#undef  getgid
#define getgid(...)     ((gid_t)0)
#undef  getegid
#define getegid(...)    ((gid_t)0)

#undef  setuid
#define setuid(u)       (errno = ENOSYS, -1)
#undef  setgid
#define setgid(g)       (errno = ENOSYS, -1)

#endif /* PICOPERL_LIBC_PICO2_UNISTD_H */
