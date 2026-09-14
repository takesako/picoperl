# ramfs: 書き込み可能インメモリファイルシステムの単体ビルド/テスト。
# まだpicoperl/romperlには組み込んでおらず、ramfs.c単体をCから検証する
# ためだけのMakefile。

CC = cc
OPTIMIZE = -Os -std=gnu89 -Wall
T_DIR = ../t

all: ramfs.o

ramfs.o: ramfs.c ramfs.h
	$(CC) -c -o $@ $(OPTIMIZE) ramfs.c

# t/ramfs_test.c は ramfs.h に依存するので -I. でカレントディレクトリ
# (ramfs/)をinclude pathに追加する。
ramfs_test: $(T_DIR)/ramfs_test.c ramfs.o
	$(CC) -o $@ $(OPTIMIZE) -I. $(T_DIR)/ramfs_test.c ramfs.o

test: ramfs_test
	./ramfs_test

clean:
	rm -f ramfs.o ramfs_test

.PHONY: all clean test
