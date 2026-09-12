/*
 * romperl の main()。picoperl-5.12.5/miniperlmain.c を土台に、この picoperl
 * ビルドでは決して真にならない分岐 (PERL_GLOBAL_STRUCT, USE_ITHREADS,
 * atarist, NO_ENV_ARRAY_IN_MAIN 等) を削って見通しを良くしたもの。
 *
 * 現時点では ROMFS 未実装のため、picoperl と同様に argv 経由で
 * -e やスクリプトパスを受け取る。ROMFS 導入後、ここを ROMFS からの
 * 読み込みに置き換える。
 */

#include "EXTERN.h"
#define PERL_IN_MINIPERLMAIN_C
#include "perl.h"

static void xs_init(pTHX);
static PerlInterpreter *my_perl;

int
main(int argc, char **argv, char **env)
{
    dVAR;
    int exitstatus;

    PERL_SYS_INIT3(&argc, &argv, &env);

    my_perl = perl_alloc();
    if (!my_perl)
        exit(1);
    perl_construct(my_perl);
    PL_perl_destruct_level = 0;
    PL_exit_flags |= PERL_EXIT_DESTRUCT_END;

    exitstatus = perl_parse(my_perl, xs_init, argc, argv, (char **)NULL);
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
    PERL_UNUSED_CONTEXT;
    dXSUB_SYS;
}
