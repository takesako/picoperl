# libc: picoperlが依存するlibc関数の代替実装(env/sort/rand/picotime/
# ctype)の単体ビルド/テスト。このディレクトリはromperlのビルド
# (-I../libc)からも直接参照される(*.hのlibcシムと、これらの強い実装を
# 同じ場所に置く)。

CC = cc
OPTIMIZE = -Os -std=gnu89 -Wall -Wextra
T_DIR = ../t

all: env.o sort.o rand.o picotime.o ctype.o

env.o: env.c env.h
	$(CC) -c -o $@ $(OPTIMIZE) env.c

sort.o: sort.c sort.h
	$(CC) -c -o $@ $(OPTIMIZE) sort.c

rand.o: rand.c rand.h
	$(CC) -c -o $@ $(OPTIMIZE) rand.c

picotime.o: picotime.c picotime.h
	$(CC) -c -o $@ $(OPTIMIZE) picotime.c

ctype.o: ctype.c picoctype.h
	$(CC) -c -o $@ $(OPTIMIZE) ctype.c

env_test: $(T_DIR)/env_test.c env.o
	$(CC) -o $@ $(OPTIMIZE) -I. $(T_DIR)/env_test.c env.o

sort_test: $(T_DIR)/sort_test.c sort.o
	$(CC) -o $@ $(OPTIMIZE) -I. $(T_DIR)/sort_test.c sort.o

rand_test: $(T_DIR)/rand_test.c rand.o
	$(CC) -o $@ $(OPTIMIZE) -I. $(T_DIR)/rand_test.c rand.o

time_test: $(T_DIR)/time_test.c picotime.o
	$(CC) -o $@ $(OPTIMIZE) -I. $(T_DIR)/time_test.c picotime.o

ctype_test: $(T_DIR)/ctype_test.c ctype.o
	$(CC) -o $@ $(OPTIMIZE) -I. $(T_DIR)/ctype_test.c ctype.o

test: env_test sort_test rand_test time_test ctype_test
	./env_test
	./sort_test
	./rand_test
	./time_test
	./ctype_test

clean:
	rm -f env.o sort.o rand.o picotime.o ctype.o env_test sort_test rand_test time_test ctype_test

.PHONY: all clean test
