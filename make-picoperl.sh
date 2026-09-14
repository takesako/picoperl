#!/bin/sh
set -eu

V=5.12.5; T=perl-$V.tar.gz; SRC=perl-$V; OUT=picoperl-$V
URL=https://www.cpan.org/src/5.0/$T; CC=${CC:-cc}; JOBS=${JOBS:-4}
OPTIMIZE="${OPTIMIZE:--Os -std=gnu89 -DNO_MATHOMS -flto -ffunction-sections -fdata-sections -I../libc}"
case $(uname -s) in
Darwin) LDFLAGS="${LDFLAGS:--flto -Wl,-dead_strip}";;
*) LDFLAGS="${LDFLAGS:--flto -Wl,--gc-sections}";;
esac

[ -f "$T" ] || curl -fL "$URL" -o "$T"
[ -d "$SRC" ] || tar xzf "$T"
rm -rf "$OUT"; mkdir "$OUT"

cd "$SRC"
for f in $(cat <<'EOF'
Artistic Copying README.micro Makefile.micro config_h.SH uconfig.sh uconfig.h
av.c av.h deb.c doio.c doop.c dump.c globals.c gv.c gv.h hv.c hv.h mro.c mg.c mg.h
op.c op.h pad.c pad.h perl.c perl.h perlapi.c perlapi.h perlio.c perlio.h perliol.h
perlsdio.h perly.c perly.h perly.y pp.c pp.h pp_ctl.c pp_hot.c pp_pack.c pp_sort.c pp_sys.c
reentr.c reentr.h regcomp.c regcomp.h regexec.c regexp.h regcharclass.h regnodes.h
run.c scope.c scope.h sv.c sv.h taint.c time64.c toke.c numeric.c locale.c mathoms.c
overload.c universal.c utf8.c utf8.h util.c util.h cop.h cv.h embed.h embedvar.h EXTERN.h
INTERN.h form.h handy.h intrpvar.h iperlsys.h keywords.h mydtrace.h nostdio.h opcode.h
opnames.h overload.h parser.h patchlevel.h perlvars.h pp_proto.h proto.h thread.h
time64.h time64_config.h unixish.h warnings.h XSUB.h perly.act perly.tab miniperlmain.c
EOF
); do cp -p "$f" "../$OUT/"; done
cp -p ../generate_uudmap.pl "../$OUT/"
cp -p ../posix_shim.c "../$OUT/"
cp -p ../stdio_shim.c "../$OUT/"
cp -p ../env_shim.c "../$OUT/"
cp -p ../sort_shim.c "../$OUT/"
cp -p ../rand_shim.c "../$OUT/"
cp -p ../time_shim.c "../$OUT/"
cp -p ../ctype_shim.c "../$OUT/"
cp -p Makefile.micro "../$OUT/Makefile"

cd "../$OUT"
chmod u+w Makefile miniperlmain.c uconfig.sh uconfig.h perl.h sv.c
perl -pi -e 's/\ball:\s+microperl\b/all: picoperl/;s/^microperl:/picoperl:/;s/-o microperl/-o picoperl/;' Makefile
perl -pi -e 's/\Q microperl generate_uudmap$(_X) uudmap.h\E/ picoperl/;' Makefile
perl -0777 -pi -e 's/^uudmap\.h: generate_uudmap.*?^# That.s it, folks!//ms' Makefile
cat >> Makefile <<'EOF'
uudmap.h bitcount.h: generate_uudmap.pl
	$(PERL) generate_uudmap.pl uudmap.h bitcount.h
EOF
# posix_shim.o: stat/unlink(パス名だけで完結する操作)をvfs(romfs+ramfs)
# 経由にリダイレクトするための弱いデフォルト実装(本物のシステムコールへの
# パススルー)。romperlは同名の強いシンボル(vfs/vfs_posix.c)でリンク時に
# 上書きする(pp_requireのromperl_find_for_compileと同じ仕組み)。
# open/close/read/write/lseek/fstatはvfs対応していない
# (TODO.md「Phase 5」参照: このビルドではsysopenがopen()で得たfdを
# 本物のfdopen()に渡す経路が必須で、fdだけvfs化しても動かないため)。
perl -pi -e 's/(uuniversal\$\(_O\) uutf8\$\(_O\) uutil\$\(_O\) uperlapi\$\(_O\))/$1 uposix_shim\$(_O)/' Makefile
cat >> Makefile <<'EOF'
uposix_shim$(_O): $(HE) posix_shim.c
	$(CC) $(CCFLAGS) -o $@ $(CFLAGS) posix_shim.c
EOF
# stdio_shim.o: fopen系(fopen/fclose/fread/fwrite/fseek/ftell/feof/
# ferror/clearerr/fflush/fgetc/fputs/fileno/fprintf)をvfs経由に
# リダイレクトするための弱いデフォルト実装。posix_shim.oと同じ
# 弱い/強いシンボルの仕組みで、romperlはvfs/vfs_stdio.cの強い実装
# (ROMFS/RAMFS経由)で上書きする。このビルド(useperlio=undef)では
# 通常のopen()がPerlIO_open=fopen()に直結しているため、fopen系だけ
# 対応すればsysopen以外のファイルI/Oは動く。
perl -pi -e 's/(uuniversal\$\(_O\) uutf8\$\(_O\) uutil\$\(_O\) uperlapi\$\(_O\) uposix_shim\$\(_O\))/$1 ustdio_shim\$(_O)/' Makefile
cat >> Makefile <<'EOF'
ustdio_shim$(_O): $(HE) stdio_shim.c
	$(CC) $(CCFLAGS) -o $@ $(CFLAGS) stdio_shim.c
EOF
# env_shim.o: getenv/putenvをvfs等と同じ弱いデフォルト実装(本物のlibc
# 関数へのパススルー)にする。plain picoperlはNV=floatの最小実装のまま
# にする方針のため、libc依存を削るのはromperl側だけでよい
# (romperl/libc/env.c参照)。posix_shim.o/stdio_shim.oと同じ弱い/強い
# シンボルの仕組み。
perl -pi -e 's/(uuniversal\$\(_O\) uutf8\$\(_O\) uutil\$\(_O\) uperlapi\$\(_O\) uposix_shim\$\(_O\) ustdio_shim\$\(_O\))/$1 uenv_shim\$(_O)/' Makefile
cat >> Makefile <<'EOF'
uenv_shim$(_O): $(HE) env_shim.c
	$(CC) $(CCFLAGS) -o $@ $(CFLAGS) env_shim.c
EOF
# sort_shim.o: qsortを内製の挿入ソート(romperl/libc/sort.c)に繋ぐための
# 弱いデフォルト実装(本物のlibc qsort(3)へのパススルー)。
# env_shim.o/posix_shim.o/stdio_shim.oと同じ弱い/強いシンボルの仕組み。
perl -pi -e 's/(uuniversal\$\(_O\) uutf8\$\(_O\) uutil\$\(_O\) uperlapi\$\(_O\) uposix_shim\$\(_O\) ustdio_shim\$\(_O\) uenv_shim\$\(_O\))/$1 usort_shim\$(_O)/' Makefile
cat >> Makefile <<'EOF'
usort_shim$(_O): $(HE) sort_shim.c
	$(CC) $(CCFLAGS) -o $@ $(CFLAGS) sort_shim.c
EOF
# rand_shim.o: rand/srandを内製PRNG(romperl/libc/rand.c)に繋ぐための
# 弱いデフォルト実装(本物のlibc rand(3)/srand(3)へのパススルー)。
# env_shim.o/posix_shim.o/stdio_shim.o/sort_shim.oと同じ弱い/強い
# シンボルの仕組み。
perl -pi -e 's/(uuniversal\$\(_O\) uutf8\$\(_O\) uutil\$\(_O\) uperlapi\$\(_O\) uposix_shim\$\(_O\) ustdio_shim\$\(_O\) uenv_shim\$\(_O\) usort_shim\$\(_O\))/$1 urand_shim\$(_O)/' Makefile
cat >> Makefile <<'EOF'
urand_shim$(_O): $(HE) rand_shim.c
	$(CC) $(CCFLAGS) -o $@ $(CFLAGS) rand_shim.c
EOF
# time_shim.o: time/localtimeを内製実装(romperl/libc/picotime.c、
# 固定エポック+ゼロから計算するUTCカレンダー変換)に繋ぐための弱い
# デフォルト実装(本物のlibc time(3)/localtime(3)へのパススルー)。
# env_shim.o/posix_shim.o/stdio_shim.o/sort_shim.o/rand_shim.oと同じ
# 弱い/強いシンボルの仕組み。
perl -pi -e 's/(uuniversal\$\(_O\) uutf8\$\(_O\) uutil\$\(_O\) uperlapi\$\(_O\) uposix_shim\$\(_O\) ustdio_shim\$\(_O\) uenv_shim\$\(_O\) usort_shim\$\(_O\) urand_shim\$\(_O\))/$1 utime_shim\$(_O)/' Makefile
cat >> Makefile <<'EOF'
utime_shim$(_O): $(HE) time_shim.c
	$(CC) $(CCFLAGS) -o $@ $(CFLAGS) time_shim.c
EOF
# ctype_shim.o: is*系/to*系(libcのlocale依存__ctype_b_loc経由)を内製実装
# (romperl/libc/ctype.c、ASCII範囲の単純な比較)に繋ぐための弱い
# デフォルト実装(本物のlibc is*(3)/to*(3)へのパススルー)。
# env_shim.o/posix_shim.o/stdio_shim.o/sort_shim.o/rand_shim.o/
# time_shim.oと同じ弱い/強いシンボルの仕組み。
perl -pi -e 's/(uuniversal\$\(_O\) uutf8\$\(_O\) uutil\$\(_O\) uperlapi\$\(_O\) uposix_shim\$\(_O\) ustdio_shim\$\(_O\) uenv_shim\$\(_O\) usort_shim\$\(_O\) urand_shim\$\(_O\) utime_shim\$\(_O\))/$1 uctype_shim\$(_O)/' Makefile
cat >> Makefile <<'EOF'
uctype_shim$(_O): $(HE) ctype_shim.c
	$(CC) $(CCFLAGS) -o $@ $(CFLAGS) ctype_shim.c
EOF
perl -0777 -pi -e 's@(    /\* Unregister our signal handler.*?)(    exitstatus = perl_destruct)@#ifndef PERL_MICRO\n$1#endif\n$2@s' miniperlmain.c
perl -MConfig -pi -e 's/^((?:short|int|long(?:dbl|long)?|ptr|double|[iun]v|u?quad|[iu]\d+|fpos|lseek)(?:size|type)|byteorder|d_quad|quadkind|use64.+|uidtype|gidtype)=.*/"$1=\x27$Config{$1}\x27"/e; s/^(d_const|i_unistd|i_fcntl)=.*/$1=\x27define\x27/' uconfig.sh
perl -MConfig -pi -e 's/^(signal_t)=.*/"$1=\x27$Config{$1}\x27"/e;' uconfig.sh
# perl -MConfig -pi -e 's/^(\w+)=.*/exists $Config{$1} ? $1."=\x27".(defined $Config{$1}?$Config{$1}:"undef")."\x27" : $&/e' uconfig.sh
perl -pi -e "s/^nvtype=.*/nvtype='float'/; s/^nvsize=.*/nvsize='4'/" uconfig.sh

# i_float='undef' だと perl.h は <float.h> をincludeしない。DBL_DIG等はperl.h内に
# フォールバック値があるため気づきにくいが、FLT_DIG等は代替値が無いためNVSIZE==4の
# 分岐でコンパイルエラーになる。glibcには<float.h>があるので素直に有効化する。
# d_dbl_digも合わせて有効化し、perl.hのDBL_DIGフォールバック定義との重複警告を防ぐ。
perl -pi -e "s/^i_float=.*/i_float='define'/; s/^d_dbl_dig=.*/d_dbl_dig='define'/" uconfig.sh

# i_syswait='undef' だと <sys/wait.h> がどこからもincludeされず、wait()の宣言が
# 無い(暗黙のK&R形式)まま本物のwait()が直接呼ばれる。libc/sys/wait.hで
# wait/waitpidを無効化しても、includeされなければ差し替わらない。有効化して
# シム経由にする(WCOREDUMP等のマクロが追加で使えるようになるだけで副作用は無い)。
perl -pi -e "s/^i_syswait=.*/i_syswait='define'/" uconfig.sh

# NVSIZE==4 の場合、Perl_sin等の関数ポインタ (NV(*)(NV)) にdouble版のsin/cos等を
# 代入すると引数/戻り値のABIが食い違い呼び出しが壊れる(pp.cのpp_sin参照)ので、
# float版libm関数(sinf等)に差し替える。
perl -0777 -pi -e 's/^(#   define Perl_cos cos\n#   define Perl_sin sin\n#   define Perl_sqrt sqrt\n#   define Perl_exp exp\n#   define Perl_log log\n#   define Perl_atan2 atan2\n#   define Perl_pow pow\n#   define Perl_floor floor\n#   define Perl_ceil ceil\n#   define Perl_fmod fmod\n#   define Perl_modf\(x,y\) modf\(x,y\)\n#   define Perl_frexp\(x,y\) frexp\(x,y\)\n)/#   if NVSIZE == 4\n#   define Perl_cos cosf\n#   define Perl_sin sinf\n#   define Perl_sqrt sqrtf\n#   define Perl_exp expf\n#   define Perl_log logf\n#   define Perl_atan2 atan2f\n#   define Perl_pow powf\n#   define Perl_floor floorf\n#   define Perl_ceil ceilf\n#   define Perl_fmod fmodf\n#   define Perl_modf(x,y) modff(x,y)\n#   define Perl_frexp(x,y) frexpf(x,y)\n#   else\n$1#   endif\n/m' perl.h

# NVSIZE==4 でも NV_DIG/NV_MANT_DIG/NV_MIN/NV_MAX/NV_EPSILON は常に DBL_* に
# ハードコードされている。これらは数値→文字列変換(sv_2pv/Gconvertのprintf精度)や
# numeric.cのPerl_my_atof2(MAX_SIG_DIGITS=NV_DIG+2)が参照するため、float基準の
# FLT_* に差し替えないと print で無意味に長い桁(倍精度昇格分のゴミ桁)が出る。
# (直前のPerl_sinパッチが挿入する「#   if NVSIZE == 4」を目印にするため、この
#  パッチは必ずPerl_sinパッチの後に実行すること)
perl -0777 -pi -e 's/(#   define NV_DIG DBL_DIG\n.*?\n)(?=#   if NVSIZE == 4\n)/#   if NVSIZE == 4\n#   define NV_DIG FLT_DIG\n#   ifdef FLT_MANT_DIG\n#       define NV_MANT_DIG FLT_MANT_DIG\n#   endif\n#   ifdef FLT_MIN\n#       define NV_MIN FLT_MIN\n#   endif\n#   ifdef FLT_MAX\n#       define NV_MAX FLT_MAX\n#   endif\n#   ifdef FLT_MIN_10_EXP\n#       define NV_MIN_10_EXP FLT_MIN_10_EXP\n#   endif\n#   ifdef FLT_MAX_10_EXP\n#       define NV_MAX_10_EXP FLT_MAX_10_EXP\n#   endif\n#   ifdef FLT_EPSILON\n#       define NV_EPSILON FLT_EPSILON\n#   endif\n#   ifdef FLT_MAX\n#       define NV_MAX FLT_MAX\n#       define NV_MIN FLT_MIN\n#   else\n#       ifdef HUGE_VALF\n#           define NV_MAX HUGE_VALF\n#       endif\n#   endif\n#   else\n$1#   endif\n/s' perl.h

# perl.hはgetuid/geteuid/getgid/getegidを無条件に素のプロトタイプとして
# 再宣言している。libc/unistd.hがこれらを関数マクロに差し替えると、
# マクロはコール式だけでなく宣言文の中の同名トークンも展開してしまうため
# `Uid_t getuid (void);` が `Uid_t ((uid_t)0);` のような壊れた宣言になる。
# シムのインクルードガードが有効な間だけこの再宣言をスキップする。
perl -0777 -pi -e 's/(Uid_t getuid \(void\);\nUid_t geteuid \(void\);\nGid_t getgid \(void\);\nGid_t getegid \(void\);\n)/#ifndef PICOPERL_LIBC_UNISTD_H\n$1#endif\n/' perl.h

# SVt_NVの空きリストはbody領域にvoid*(8byte)を書き込んで繋ぐ(sv.cのS_more_bodies)。
# NV=float(4byte)だとポインタサイズ未満になり隣接領域を破壊するため、
# arenaに積むbody_sizeはsizeof(NV)とsizeof(char*)の大きい方にする。
perl -0777 -pi -e 's/\{ sizeof\(NV\), sizeof\(NV\), 0, SVt_NV, FALSE, HADNV, HASARENA,\n(\s*)FIT_ARENA\(0, sizeof\(NV\)\) \},/{ (sizeof(NV)<sizeof(char*)?sizeof(char*):sizeof(NV)), sizeof(NV), 0, SVt_NV, FALSE, HADNV, HASARENA,\n$1FIT_ARENA(0, (sizeof(NV)<sizeof(char*)?sizeof(char*):sizeof(NV))) },/' sv.c

# pp_require を直接ROMFSに繋いでtrue zero-copyにするためのフック点。
# romperlはpicoperl-5.12.5の.oをコピーせず参照するだけなので、picoperlと
# romperlは同じupp_ctl.oを共有する。#ifdefでの出し分けができないため、
# 「常にNULLを返す弱いシンボル」をここに置き、romperl側だけromfs_compile.o
# の強いシンボルでリンク時に上書きする(picoperl.exeはromfs_compile.oを
# リンクしないので弱いstubのまま=挙動は今まで通り変わらない)。
perl -0777 -pi -e 's@(#endif /\* !PERL_DISABLE_PMC \*/\n\n)(PP\(pp_require\))@$1__attribute__((weak))\nconst void *\nromperl_find_for_compile(const char *name, unsigned long *out_len)\n{\n    PERL_UNUSED_ARG(name);\n    PERL_UNUSED_ARG(out_len);\n    return NULL;\n}\n\n$2@' pp_ctl.c

# pp_require内でROMFSから見つけたソースを保持するSV変数を追加。
perl -pi -e 's/(\s+SV \*hook_sv = NULL;)/$1\n    SV *romsv = NULL; \/* romperl: ROMFSから見つかったソース(ゼロコピー) *\//' pp_ctl.c

# %INC確認の直後・@INC探索が始まる前にROMFSを直接調べる。見つかれば
# romsvにSvLEN=0(所有権なし)のSVを組み立て、ROMFSイメージ上のバイト列を
# 直接指す。tryrsfp/tryname周りの既存ロジックは変更せず、!romsvガードで
# 素通りさせる(下のパッチ参照)。
perl -0777 -pi -e 's@(\n    /\* prepare to compile file \*/\n)@\n    \/* romperl: INC探索より前にROMFSを直接調べる *\/\n    {\n        unsigned long romlen = 0;\n        const void *romptr = romperl_find_for_compile(name, \&romlen);\n        if (romptr) {\n            romsv = newSV_type(SVt_PV);\n            SvPV_set(romsv, (char *)romptr);\n            SvCUR_set(romsv, (STRLEN)romlen);\n            SvLEN_set(romsv, 0);\n            SvPOK_on(romsv);\n            tryname = name;\n        }\n    }\n$1@' pp_ctl.c

# ROMFSで見つかった場合は「@INC探索に入る」条件と「見つからずDIEする」
# 条件の両方(if (!tryrsfp) { が2箇所)をスキップさせる。
perl -pi -e 's/if \(!tryrsfp\) \{/if (!tryrsfp \&\& !romsv) {/g' pp_ctl.c

# lex_start()はline SVがSvREADONLYでなく最後のバイトが';'ならコピーせず
# そのSVをそのまま使う(Perl_lex_start@toke.c)。romsvがあればそれを渡し、
# lex_start内部でのSvREFCNT_inc分をここでdecして返す。
perl -pi -e 's/(\s+)lex_start\(NULL, tryrsfp, TRUE\);/$1lex_start(romsv ? romsv : NULL, tryrsfp, TRUE);\n$1if (romsv) SvREFCNT_dec(romsv);/' pp_ctl.c

make regen_uconfig
make clean
make -j"$JOBS" CC="$CC" LD="$CC" OPTIMIZE="$OPTIMIZE" LDFLAGS="$LDFLAGS"
./picoperl -e 'print "picoperl $^V OK\n"'
printf 'binary: '; wc -c < picoperl
./picoperl ../t/test-float.pl
./picoperl ../t/test-noproc.pl
./picoperl ../t/test-env.pl
./picoperl ../t/test-ctype.pl
