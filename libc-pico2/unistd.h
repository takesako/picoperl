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

/*
 * unlink はパス名だけを扱い、fd/FILE*を経由しないため単独でvfs(romfs+
 * ramfs)経由のpicoperl_unlink()にリダイレクトできる。
 * plain picoperlではposix_shim.cの弱いデフォルト実装(本物のシステム
 * コールへのパススルー)が使われ、romperlではvfs/vfs_posix.cの強い実装
 * (ROMFS/RAMFS経由)がリンク時に上書きする。
 *
 * open/close/read/write/lseekは意図的にリダイレクトしていない。
 * このビルド(useperlio=undef)では sysopen 等が
 * `PerlLIO_open3()`(=open)で得たfdを直後に`PerlSIO_fdopen()`(本物の
 * fdopen)へ渡してFILE*化するため、open()だけをvfs用の偽fdにリダイレクト
 * すると本物のfdopen()がEBADFで失敗する("Bad file descriptor")。
 * fdopen/fopen以降のstdio全体(fread/fwrite/fclose/fseek等)を合わせて
 * vfs対応させない限りopen側だけを差し替えても動かないため、TODOの
 * 「stdioをPerlIO経由でUARTに直結」と合わせて後で一括対応する
 * (詳細はTODO.md「Phase 5」参照)。
 */
extern int  picoperl_unlink(const char *path);

#undef  unlink
#define unlink(path)            picoperl_unlink((path))

#endif /* PICOPERL_LIBC_PICO2_UNISTD_H */
