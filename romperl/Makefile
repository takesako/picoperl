# romperl: ../picoperl-5.12.5 が生成した .o をコピーせずそのまま参照してリンクし、
# main() だけを romperl 用の main.c に差し替えたバイナリを作る。
#
# 前提: 先に ../make-picoperl.sh を実行して ../picoperl-5.12.5/*.o を
# 生成しておくこと (commit 前の `make clean` はこの .o も消してしまうので
# romperl をビルドし直す前に ../make-picoperl.sh を再実行すること)。

PICOPERL_DIR = ../picoperl-5.12.5
RAMFS_DIR = ../ramfs
VFS_DIR = ../vfs
LIBC_DIR = ../libc
CC = cc
OPTIMIZE = -Os -std=gnu89 -DNO_MATHOMS -flto -ffunction-sections -fdata-sections \
	-I$(LIBC_DIR) -I$(PICOPERL_DIR) -I$(RAMFS_DIR) -I$(VFS_DIR) -I.
CFLAGS = -DPERL_CORE -DPERL_MICRO -DSTANDARD_C -DPERL_USE_SAFE_PUTENV $(OPTIMIZE)
LDFLAGS = -flto=auto -Wl,--gc-sections

# picoperl本体の.o一式から、main()を含むuperlmain.oだけ除外して使う。
PICOPERL_OBJS = $(filter-out %/uperlmain.o,$(wildcard $(PICOPERL_DIR)/*.o))

all: romperl

# romfs.o/romfs_data.o/ramfs.o/vfs.oはperl.hに依存しないのでOPTIMIZEのみで
# 足りるが、romfs_xs.oはperl.h/XSUB.hを使うためmain.cと同じCFLAGS
# (PERL_CORE等)でコンパイルする。異なるdefineでABIがずれるのを防ぐため。
# vfs_posix.o/vfs_stdio.o/env.o/sort.o/rand.o/time.oは、picoperl-5.12.5/
# posix_shim.o・stdio_shim.o・env_shim.o・sort_shim.o・rand_shim.o・
# time_shim.o(弱いデフォルト、本物のlibc関数へのパススルー)をリンク時に
# 上書きし、stat/unlink(vfs_posix.o)・fopen系(vfs_stdio.o)・
# getenv/putenv(env.o)・qsort(sort.o)・rand/srand(rand.o)・
# time/localtime(time.o)をROMFS/RAMFS/自前実装経由にする(pp_requireの
# romperl_find_for_compileと同じ弱い/強いシンボルの仕組み)。
# open/close/read/write/lseek/fstatは対象外(TODO.md「Phase 5」参照)。
# libc依存を削るのはromperl側だけでよく、plain picoperlはNV=floatの
# 最小実装のままにする方針のため、posix_shim.c/stdio_shim.c/
# env_shim.c/sort_shim.c/rand_shim.c/time_shim.cはいずれも弱い
# デフォルトのまま。
romperl: uperlmain.o romfs.o romfs_data.o romfs_xs.o romfs_compile.o ramfs.o vfs.o vfs_posix.o vfs_stdio.o env.o sort.o rand.o time.o
	@if [ -z "$(strip $(PICOPERL_OBJS))" ]; then \
		echo "error: $(PICOPERL_DIR)/*.o が無い。先に ../make-picoperl.sh を実行すること" >&2; \
		exit 1; \
	fi
	$(CC) -o $@ uperlmain.o romfs.o romfs_data.o romfs_xs.o romfs_compile.o ramfs.o vfs.o vfs_posix.o vfs_stdio.o env.o sort.o rand.o time.o $(PICOPERL_OBJS) $(LDFLAGS) -lm

uperlmain.o: main.c romfs.h $(LIBC_DIR)/env.h
	$(CC) -c -o $@ $(CFLAGS) main.c

romfs_xs.o: romfs_xs.c romfs.h
	$(CC) -c -o $@ $(CFLAGS) romfs_xs.c

# picoperl-5.12.5/pp_ctl.c (make-picoperl.shがパッチ)にある弱いシンボル
# romperl_find_for_compile を、ここでリンクする強いシンボルで上書きし、
# pp_requireを直接ROMFSに繋ぐ。perl.h不要のプレーンCなのでromfs.oと同じ
# OPTIMIZEのみでコンパイル可。
romfs_compile.o: romfs_compile.c romfs.h
	$(CC) -c -o $@ $(OPTIMIZE) romfs_compile.c

# rootfs/lib/*.pm は perl-5.12.5内の各*.pmを ../minify.pl (PPIでコメント/POD/
# 空白を削るミニファイア) にかけて生成する。コミットはせず、ビルドの都度
# 作り直す(root.romfsやromfs_data.cと同じ扱い)。
# CGI.pm/CGI::Util は core lib/ ではなく cpan/CGI/lib/ 、constant は
# dist/constant/lib/ に置かれているため、それぞれ個別ルールで対応する。
PERL_LIB = ../perl-5.12.5/lib
CPAN_CGI_LIB = ../perl-5.12.5/cpan/CGI/lib
DIST_CONSTANT_LIB = ../perl-5.12.5/dist/constant/lib
MINIFY = ../minify.pl

ROOTFS_PM = \
	rootfs/lib/strict.pm \
	rootfs/lib/warnings.pm \
	rootfs/lib/warnings/register.pm \
	rootfs/lib/Carp.pm \
	rootfs/lib/Exporter.pm \
	rootfs/lib/Exporter/Heavy.pm \
	rootfs/lib/feature.pm \
	rootfs/lib/overload.pm \
	rootfs/lib/vars.pm \
	rootfs/lib/constant.pm \
	rootfs/lib/CGI.pm \
	rootfs/lib/CGI/Util.pm

rootfs/lib/%.pm: $(PERL_LIB)/%.pm $(MINIFY)
	@mkdir -p $(dir $@)
	perl $(MINIFY) < $< > $@

rootfs/lib/CGI.pm: $(CPAN_CGI_LIB)/CGI.pm $(MINIFY)
	@mkdir -p $(dir $@)
	perl $(MINIFY) < $< > $@

rootfs/lib/CGI/Util.pm: $(CPAN_CGI_LIB)/CGI/Util.pm $(MINIFY)
	@mkdir -p $(dir $@)
	perl $(MINIFY) < $< > $@

rootfs/lib/constant.pm: $(DIST_CONSTANT_LIB)/constant.pm $(MINIFY)
	@mkdir -p $(dir $@)
	perl $(MINIFY) < $< > $@

# ROMFSイメージをrootfs/から生成し、Cソースとして埋め込む。rootfs/lib/以下は
# 上のルールで生成するので、素のfindでは拾わずROOTFS_PMを明示的に依存に含める。
ROOTFS_STATIC_FILES = $(shell find rootfs -type f -not -path 'rootfs/lib/*')

root.romfs: mkromfs.pl $(ROOTFS_PM) $(ROOTFS_STATIC_FILES)
	./mkromfs.pl rootfs root.romfs

romfs_data.c: mkromfs.pl root.romfs
	./mkromfs.pl --carray root.romfs romfs_data.c romfs_image

romfs_data.o: romfs_data.c
	$(CC) -c -o $@ $(OPTIMIZE) romfs_data.c

romfs.o: romfs.c romfs.h
	$(CC) -c -o $@ $(OPTIMIZE) romfs.c

ramfs.o: $(RAMFS_DIR)/ramfs.c $(RAMFS_DIR)/ramfs.h
	$(CC) -c -o $@ $(OPTIMIZE) $(RAMFS_DIR)/ramfs.c

vfs.o: $(VFS_DIR)/vfs.c $(VFS_DIR)/vfs.h romfs.h $(RAMFS_DIR)/ramfs.h
	$(CC) -c -o $@ $(OPTIMIZE) $(VFS_DIR)/vfs.c

vfs_posix.o: $(VFS_DIR)/vfs_posix.c $(VFS_DIR)/vfs_posix.h $(VFS_DIR)/vfs.h
	$(CC) -c -o $@ $(OPTIMIZE) $(VFS_DIR)/vfs_posix.c

vfs_stdio.o: $(VFS_DIR)/vfs_stdio.c $(VFS_DIR)/vfs_stdio.h $(VFS_DIR)/vfs.h
	$(CC) -c -o $@ $(OPTIMIZE) $(VFS_DIR)/vfs_stdio.c

# env.o/sort.o/rand.o/time.oは、libc/(getenv・putenv・qsort・rand・
# srand・time・localtimeに依存しない自前実装。*.hのlibcシムと同じ
# ../libc/ に置かれている)の強い実装。romfs/ramfs/vfsとは無関係の
# 独立したモジュールだが、同じ弱い/強いシンボルの仕組みで
# picoperl-5.12.5/*_shim.oを上書きする。
env.o: $(LIBC_DIR)/env.c $(LIBC_DIR)/env.h
	$(CC) -c -o $@ $(OPTIMIZE) $(LIBC_DIR)/env.c

sort.o: $(LIBC_DIR)/sort.c $(LIBC_DIR)/sort.h
	$(CC) -c -o $@ $(OPTIMIZE) $(LIBC_DIR)/sort.c

rand.o: $(LIBC_DIR)/rand.c $(LIBC_DIR)/rand.h
	$(CC) -c -o $@ $(OPTIMIZE) $(LIBC_DIR)/rand.c

time.o: $(LIBC_DIR)/picotime.c $(LIBC_DIR)/picotime.h
	$(CC) -c -o $@ $(OPTIMIZE) $(LIBC_DIR)/picotime.c

# romfs.c/romfs_data.cのAPI単体テスト (Perl統合前の動作確認用)。
# t/romfs_test.c は romfs.h に依存するので -I. でカレントディレクトリ
# (romperl/)をinclude pathに追加する。
T_DIR = ../t

romfs_test: $(T_DIR)/romfs_test.c romfs.o romfs_data.o
	$(CC) -o $@ $(OPTIMIZE) -I. $(T_DIR)/romfs_test.c romfs.o romfs_data.o $(LDFLAGS)

test-romfs: romfs_test
	./romfs_test

# romperl本体を使ったテスト一式。既存のpicoperl向けテストが romperl でも
# 通ること(回帰確認)と、ROMFS経由のrequire/useが動くことを確認する。
test-inc-require: romperl
	./romperl $(T_DIR)/test-inc-require.pl

# LD_PRELOAD mallocトレーサ(t/malloctrace.c)を使い、pp_requireを直接
# ROMFSに繋いだtrue zero-copy経路が実際にファイルサイズ分のmalloc/realloc
# を発生させていないことを実測で検証する(t/test-zerocopy.sh参照)。
test-zerocopy: romperl $(ROOTFS_PM)
	$(T_DIR)/test-zerocopy.sh

# open($fh,mode,path)/print/read/closeがvfs(romfs+ramfs)経由で動くことを
# 確認するPerl統合テスト。
test-stdio: romperl
	./romperl $(T_DIR)/test-stdio.pl

test: romperl test-romfs test-inc-require test-zerocopy test-stdio
	./romperl $(T_DIR)/test-float.pl
	./romperl $(T_DIR)/test-noproc.pl
	./romperl $(T_DIR)/test-env.pl
	./romperl $(T_DIR)/test-time.pl

clean:
	rm -f uperlmain.o romperl romfs.o romfs_data.o romfs_data.c root.romfs romfs_test.o romfs_test romfs_xs.o romfs_compile.o
	rm -f ramfs.o vfs.o vfs_posix.o vfs_stdio.o env.o sort.o rand.o time.o
	rm -f $(ROOTFS_PM)
	rm -f $(T_DIR)/malloctrace.so
	rmdir rootfs/lib/Exporter rootfs/lib/warnings rootfs/lib/CGI rootfs/lib 2>/dev/null || true

.PHONY: all clean test test-romfs test-inc-require test-zerocopy test-stdio
