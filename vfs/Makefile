# vfs: romfs(read-only, ../romperl)とramfs(read-write, ../ramfs)を
# 束ねるディスパッチ層の単体ビルド/テスト。まだpicoperl/romperlには
# 組み込んでおらず、vfs.c単体をCから検証するためだけのMakefile。

CC = cc
OPTIMIZE = -Os -std=gnu89 -Wall -Wextra -I../ramfs -I../romperl
T_DIR = ../t
RAMFS_DIR = ../ramfs
ROMFS_DIR = ../romperl

all: vfs.o

vfs.o: vfs.c vfs.h
	$(CC) -c -o $@ $(OPTIMIZE) vfs.c

vfs_posix.o: vfs_posix.c vfs_posix.h vfs.h
	$(CC) -c -o $@ $(OPTIMIZE) vfs_posix.c

vfs_stdio.o: vfs_stdio.c vfs_stdio.h vfs.h
	$(CC) -c -o $@ $(OPTIMIZE) vfs_stdio.c

ramfs.o: $(RAMFS_DIR)/ramfs.c $(RAMFS_DIR)/ramfs.h
	$(CC) -c -o $@ $(OPTIMIZE) $(RAMFS_DIR)/ramfs.c

romfs.o: $(ROMFS_DIR)/romfs.c $(ROMFS_DIR)/romfs.h
	$(CC) -c -o $@ $(OPTIMIZE) $(ROMFS_DIR)/romfs.c

# テスト用のromfsイメージ。romfs優先/フォールバックの境界を確認するため
# testdata/readonly.txt 1つだけを埋め込む(romperl/mkromfs.plを再利用、
# フォーマットはromperl/romfs.hと共通のため生成ツールも共有できる)。
vfs_test.romfs: $(ROMFS_DIR)/mkromfs.pl testdata/readonly.txt
	$(ROMFS_DIR)/mkromfs.pl testdata vfs_test.romfs

vfs_test_romfs_data.c: $(ROMFS_DIR)/mkromfs.pl vfs_test.romfs
	$(ROMFS_DIR)/mkromfs.pl --carray vfs_test.romfs vfs_test_romfs_data.c vfs_test_romfs_image

vfs_test_romfs_data.o: vfs_test_romfs_data.c
	$(CC) -c -o $@ $(OPTIMIZE) vfs_test_romfs_data.c

# t/vfs_test.c は vfs.h/ramfs.h/romfs.h に依存するので -I. で
# カレントディレクトリ(vfs/)をinclude pathに追加する。
vfs_test: $(T_DIR)/vfs_test.c vfs.o ramfs.o romfs.o vfs_test_romfs_data.o
	$(CC) -o $@ $(OPTIMIZE) -I. $(T_DIR)/vfs_test.c vfs.o ramfs.o romfs.o vfs_test_romfs_data.o

# t/vfs_posix_test.c・t/vfs_stdio_test.c も同じromfsイメージ
# (vfs_test_romfs_data.o)を使い回す。
vfs_posix_test: $(T_DIR)/vfs_posix_test.c vfs.o vfs_posix.o ramfs.o romfs.o vfs_test_romfs_data.o
	$(CC) -o $@ $(OPTIMIZE) -I. $(T_DIR)/vfs_posix_test.c vfs.o vfs_posix.o ramfs.o romfs.o vfs_test_romfs_data.o

vfs_stdio_test: $(T_DIR)/vfs_stdio_test.c vfs.o vfs_stdio.o ramfs.o romfs.o vfs_test_romfs_data.o
	$(CC) -o $@ $(OPTIMIZE) -I. $(T_DIR)/vfs_stdio_test.c vfs.o vfs_stdio.o ramfs.o romfs.o vfs_test_romfs_data.o

test: vfs_test vfs_posix_test vfs_stdio_test
	./vfs_test
	./vfs_posix_test
	./vfs_stdio_test

clean:
	rm -f vfs.o vfs_posix.o vfs_stdio.o ramfs.o romfs.o vfs_test_romfs_data.o vfs_test_romfs_data.c vfs_test.romfs vfs_test vfs_posix_test vfs_stdio_test

.PHONY: all clean test
