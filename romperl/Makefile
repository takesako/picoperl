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

romperl: uperlmain.o
	@if [ -z "$(strip $(PICOPERL_OBJS))" ]; then \
		echo "error: $(PICOPERL_DIR)/*.o が無い。先に ../make-picoperl.sh を実行すること" >&2; \
		exit 1; \
	fi
	$(CC) -o $@ uperlmain.o $(PICOPERL_OBJS) $(LDFLAGS) -lm

uperlmain.o: main.c
	$(CC) -c -o $@ $(CFLAGS) main.c

clean:
	rm -f uperlmain.o romperl

.PHONY: all clean
