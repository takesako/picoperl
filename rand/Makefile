# rand: libcのrand(3)/srand(3)に依存しないPRNGの単体ビルド/テスト。

CC = cc
OPTIMIZE = -Os -std=gnu89 -Wall -Wextra
T_DIR = ../t

all: rand.o

rand.o: rand.c rand.h
	$(CC) -c -o $@ $(OPTIMIZE) rand.c

rand_test: $(T_DIR)/rand_test.c rand.o
	$(CC) -o $@ $(OPTIMIZE) -I. $(T_DIR)/rand_test.c rand.o

test: rand_test
	./rand_test

clean:
	rm -f rand.o rand_test

.PHONY: all clean test
