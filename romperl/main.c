/*
 * romperl の main()。picoperl-5.12.5/miniperlmain.c を土台に、この picoperl
 * ビルドでは決して真にならない分岐 (PERL_GLOBAL_STRUCT, USE_ITHREADS,
 * atarist, NO_ENV_ARRAY_IN_MAIN 等) を削って見通しを良くしたもの。
 *
 * -e やスクリプトパスは picoperl と同様に argv 経由でディスクから読む。
 * ROMFS は @INC フック(romfs_xs.c の Romperl::Boot::import 参照)経由での
 * require/use にのみ接続していて、picoperl-5.12.5 側(doio.c/perlio.c)は
 * 一切変更していない。
 *
 * フック自体は「BEGIN時にpush @INCする」だけのPerlコードで、これは
 * xs_init()から直接eval_pvしても動かない(パーサの初期化がまだ終わって
 * おらずクラッシュする)。そのためargvの先頭に "-MRomperl::Boot" を
 * 挿入し、通常の use 文と同じ(かつ安全な)タイミングでフックが
 * インストールされるようにしている。
 */

#include "EXTERN.h"
#define PERL_IN_MINIPERLMAIN_C
#include "perl.h"

#include "romfs.h"

extern const unsigned char romfs_image[];
extern const unsigned long romfs_image_size;
extern void romperl_xs_init(pTHX);

static void xs_init(pTHX);
static PerlInterpreter *my_perl;

int
main(int argc, char **argv, char **env)
{
    dVAR;
    int exitstatus;
    char **boot_argv;
    int boot_argc = argc + 1;
    int i;

    PERL_SYS_INIT3(&argc, &argv, &env);

    if (!romfs_init(romfs_image, romfs_image_size)) {
        fprintf(stderr, "romperl: embedded ROMFS image is invalid\n");
        exit(1);
    }

    boot_argv = malloc(sizeof(char *) * (boot_argc + 1));
    if (!boot_argv) {
        perror("romperl: malloc");
        exit(1);
    }
    boot_argv[0] = argv[0];
    boot_argv[1] = (char *)"-MRomperl::Boot";
    for (i = 1; i < argc; i++)
        boot_argv[i + 1] = argv[i];
    boot_argv[boot_argc] = NULL;

    my_perl = perl_alloc();
    if (!my_perl)
        exit(1);
    perl_construct(my_perl);
    PL_perl_destruct_level = 0;
    PL_exit_flags |= PERL_EXIT_DESTRUCT_END;

    exitstatus = perl_parse(my_perl, xs_init, boot_argc, boot_argv, (char **)NULL);
    if (!exitstatus)
        exitstatus = perl_run(my_perl);

    perl_destruct(my_perl);
    perl_free(my_perl);

    PERL_SYS_TERM();

    exit(exitstatus);
    return exitstatus;
}

static void
xs_init(pTHX)
{
    dXSUB_SYS;
    romperl_xs_init(aTHX);
}
