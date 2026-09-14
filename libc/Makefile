# libc: picoperlが依存するlibc関数の代替実装(env/sort/rand/picotime)の
# 単体ビルド/テスト。このディレクトリはromperlのビルド(-I../libc)からも
# 直接参照される(*.hのlibcシムと、これらの強い実装を同じ場所に置く)。

CC = cc
OPTIMIZE = -Os -std=gnu89 -Wall -Wextra
T_DIR = ../t

all: env.o sort.o rand.o picotime.o

env.o: env.c env.h
	$(CC) -c -o $@ $(OPTIMIZE) env.c

sort.o: sort.c sort.h
	$(CC) -c -o $@ $(OPTIMIZE) sort.c

rand.o: rand.c rand.h
	$(CC) -c -o $@ $(OPTIMIZE) rand.c

picotime.o: picotime.c picotime.h
	$(CC) -c -o $@ $(OPTIMIZE) picotime.c

env_test: $(T_DIR)/env_test.c env.o
	$(CC) -o $@ $(OPTIMIZE) -I. $(T_DIR)/env_test.c env.o

sort_test: $(T_DIR)/sort_test.c sort.o
	$(CC) -o $@ $(OPTIMIZE) -I. $(T_DIR)/sort_test.c sort.o

rand_test: $(T_DIR)/rand_test.c rand.o
	$(CC) -o $@ $(OPTIMIZE) -I. $(T_DIR)/rand_test.c rand.o

time_test: $(T_DIR)/time_test.c picotime.o
	$(CC) -o $@ $(OPTIMIZE) -I. $(T_DIR)/time_test.c picotime.o

test: env_test sort_test rand_test time_test
	./env_test
	./sort_test
	./rand_test
	./time_test

clean:
	rm -f env.o sort.o rand.o picotime.o env_test sort_test rand_test time_test

.PHONY: all clean test
