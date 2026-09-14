# time: libcのtime(3)/localtime(3)に依存しない実装の単体ビルド/テスト。

CC = cc
OPTIMIZE = -Os -std=gnu89 -Wall -Wextra
T_DIR = ../t

all: picotime.o

picotime.o: picotime.c picotime.h
	$(CC) -c -o $@ $(OPTIMIZE) picotime.c

time_test: $(T_DIR)/time_test.c picotime.o
	$(CC) -o $@ $(OPTIMIZE) -I. $(T_DIR)/time_test.c picotime.o

test: time_test
	./time_test

clean:
	rm -f picotime.o time_test

.PHONY: all clean test
