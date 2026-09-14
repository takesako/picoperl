# env: libcのgetenv/putenvに依存しない環境変数ストアの単体ビルド/テスト。

CC = cc
OPTIMIZE = -Os -std=gnu89 -Wall -Wextra
T_DIR = ../t

all: env.o

env.o: env.c env.h
	$(CC) -c -o $@ $(OPTIMIZE) env.c

env_test: $(T_DIR)/env_test.c env.o
	$(CC) -o $@ $(OPTIMIZE) -I. $(T_DIR)/env_test.c env.o

test: env_test
	./env_test

clean:
	rm -f env.o env_test

.PHONY: all clean test
