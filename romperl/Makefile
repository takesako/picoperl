# romperl: ../picoperl-5.12.5 が生成した .o をコピーせずそのまま参照してリンクし、
# main() だけを romperl 用の main.c に差し替えたバイナリを作る。
#
# 前提: 先に ../make-picoperl.sh を実行して ../picoperl-5.12.5/*.o を
# 生成しておくこと (commit 前の `make clean` はこの .o も消してしまうので
# romperl をビルドし直す前に ../make-picoperl.sh を再実行すること)。

PICOPERL_DIR = ../picoperl-5.12.5
CC = cc
OPTIMIZE = -Os -std=gnu89 -DNO_MATHOMS -flto -ffunction-sections -fdata-sections \
	-I../libc-pico2 -I$(PICOPERL_DIR)
CFLAGS = -DPERL_CORE -DPERL_MICRO -DSTANDARD_C -DPERL_USE_SAFE_PUTENV $(OPTIMIZE)
LDFLAGS = -flto -Wl,--gc-sections

# picoperl本体の.o一式から、main()を含むuperlmain.oだけ除外して使う。
PICOPERL_OBJS = $(filter-out %/uperlmain.o,$(wildcard $(PICOPERL_DIR)/*.o))

all: romperl

# romfs.o/romfs_data.oはromfs.h以外picoperlのヘッダに依存しないのでOPTIMIZEのみ
# で足りるが、romfs_xs.oはperl.h/XSUB.hを使うためmain.cと同じCFLAGS
# (PERL_CORE等)でコンパイルする。異なるdefineでABIがずれるのを防ぐため。
romperl: uperlmain.o romfs.o romfs_data.o romfs_xs.o
	@if [ -z "$(strip $(PICOPERL_OBJS))" ]; then \
		echo "error: $(PICOPERL_DIR)/*.o が無い。先に ../make-picoperl.sh を実行すること" >&2; \
		exit 1; \
	fi
	$(CC) -o $@ uperlmain.o romfs.o romfs_data.o romfs_xs.o $(PICOPERL_OBJS) $(LDFLAGS) -lm

uperlmain.o: main.c romfs.h
	$(CC) -c -o $@ $(CFLAGS) main.c

romfs_xs.o: romfs_xs.c romfs.h
	$(CC) -c -o $@ $(CFLAGS) romfs_xs.c

# ROMFSイメージをrootfs/から生成し、Cソースとして埋め込む。
ROOTFS_FILES = $(shell find rootfs -type f)

root.romfs: mkromfs.pl $(ROOTFS_FILES)
	./mkromfs.pl rootfs root.romfs

romfs_data.c: mkromfs.pl root.romfs
	./mkromfs.pl --carray root.romfs romfs_data.c romfs_image

romfs_data.o: romfs_data.c
	$(CC) -c -o $@ $(OPTIMIZE) romfs_data.c

romfs.o: romfs.c romfs.h
	$(CC) -c -o $@ $(OPTIMIZE) romfs.c

# romfs.c/romfs_data.cのAPI単体テスト (Perl統合前の動作確認用)。
romfs_test: romfs_test.c romfs.o romfs_data.o
	$(CC) -o $@ $(OPTIMIZE) romfs_test.c romfs.o romfs_data.o $(LDFLAGS)

test-romfs: romfs_test
	./romfs_test

# romperl本体を使ったテスト一式。既存のpicoperl向けテストが romperl でも
# 通ること(回帰確認)と、ROMFS経由のrequire/useが動くことを確認する。
test-inc-require: romperl
	./romperl test-inc-require.pl

test: romperl test-romfs test-inc-require
	./romperl ../test-float.pl
	./romperl ../test-noproc.pl

clean:
	rm -f uperlmain.o romperl romfs.o romfs_data.o romfs_data.c root.romfs romfs_test.o romfs_test romfs_xs.o

.PHONY: all clean test test-romfs test-inc-require
