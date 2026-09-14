# sort: libcのqsort(3)に依存しない挿入ソートの単体ビルド/テスト。

CC = cc
OPTIMIZE = -Os -std=gnu89 -Wall -Wextra
T_DIR = ../t

all: sort.o

sort.o: sort.c sort.h
	$(CC) -c -o $@ $(OPTIMIZE) sort.c

sort_test: $(T_DIR)/sort_test.c sort.o
	$(CC) -o $@ $(OPTIMIZE) -I. $(T_DIR)/sort_test.c sort.o

test: sort_test
	./sort_test

clean:
	rm -f sort.o sort_test

.PHONY: all clean test
