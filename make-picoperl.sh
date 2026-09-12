#!/bin/sh
set -eu

V=5.12.5; T=perl-$V.tar.gz; SRC=perl-$V; OUT=picoperl-$V
URL=https://www.cpan.org/src/5.0/$T; CC=${CC:-cc}; JOBS=${JOBS:-4}
COPT="${COPT:--Os -std=gnu89 -DNO_MATHOMS -flto -ffunction-sections -fdata-sections}"
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
perl -0777 -pi -e 's@(    /\* Unregister our signal handler.*?)(    exitstatus = perl_destruct)@#ifndef PERL_MICRO\n$1#endif\n$2@s' miniperlmain.c
perl -MConfig -pi -e 's/^((?:short|int|long(?:dbl|long)?|ptr|double|[iun]v|u?quad|[iu]\d+|fpos|lseek)(?:size|type)|byteorder|d_quad|quadkind|use64.+|uidtype|gidtype)=.*/"$1=\x27$Config{$1}\x27"/e; s/^(d_const|i_unistd|i_fcntl)=.*/$1=\x27define\x27/' uconfig.sh
perl -MConfig -pi -e 's/^(signal_t)=.*/"$1=\x27$Config{$1}\x27"/e;' uconfig.sh
# perl -MConfig -pi -e 's/^(\w+)=.*/exists $Config{$1} ? $1."=\x27".(defined $Config{$1}?$Config{$1}:"undef")."\x27" : $&/e' uconfig.sh
perl -pi -e "s/^nvtype=.*/nvtype='float'/; s/^nvsize=.*/nvsize='4'/" uconfig.sh

# NVSIZE==4 の場合、Perl_sin等の関数ポインタ (NV(*)(NV)) にdouble版のsin/cos等を
# 代入すると引数/戻り値のABIが食い違い呼び出しが壊れる(pp.cのpp_sin参照)ので、
# float版libm関数(sinf等)に差し替える。
perl -0777 -pi -e 's/^(#   define Perl_cos cos\n#   define Perl_sin sin\n#   define Perl_sqrt sqrt\n#   define Perl_exp exp\n#   define Perl_log log\n#   define Perl_atan2 atan2\n#   define Perl_pow pow\n#   define Perl_floor floor\n#   define Perl_ceil ceil\n#   define Perl_fmod fmod\n#   define Perl_modf\(x,y\) modf\(x,y\)\n#   define Perl_frexp\(x,y\) frexp\(x,y\)\n)/#   if NVSIZE == 4\n#   define Perl_cos cosf\n#   define Perl_sin sinf\n#   define Perl_sqrt sqrtf\n#   define Perl_exp expf\n#   define Perl_log logf\n#   define Perl_atan2 atan2f\n#   define Perl_pow powf\n#   define Perl_floor floorf\n#   define Perl_ceil ceilf\n#   define Perl_fmod fmodf\n#   define Perl_modf(x,y) modff(x,y)\n#   define Perl_frexp(x,y) frexpf(x,y)\n#   else\n$1#   endif\n/m' perl.h

# SVt_NVの空きリストはbody領域にvoid*(8byte)を書き込んで繋ぐ(sv.cのS_more_bodies)。
# NV=float(4byte)だとポインタサイズ未満になり隣接領域を破壊するため、
# arenaに積むbody_sizeはsizeof(NV)とsizeof(char*)の大きい方にする。
perl -0777 -pi -e 's/\{ sizeof\(NV\), sizeof\(NV\), 0, SVt_NV, FALSE, HADNV, HASARENA,\n(\s*)FIT_ARENA\(0, sizeof\(NV\)\) \},/{ (sizeof(NV)<sizeof(char*)?sizeof(char*):sizeof(NV)), sizeof(NV), 0, SVt_NV, FALSE, HADNV, HASARENA,\n$1FIT_ARENA(0, (sizeof(NV)<sizeof(char*)?sizeof(char*):sizeof(NV))) },/' sv.c

make regen_uconfig
make clean
make -j"$JOBS" CC="$CC" LD="$CC" OPTIMIZE="$COPT" LDFLAGS="$LDFLAGS"
./picoperl -e 'print "picoperl $^V OK\n"'
printf 'binary: '; wc -c < picoperl
./picoperl ../test-float.pl
