# cm33: Phase 7(arm-none-eabi / Cortex-M33)のベアメタル最小基盤。
# 現時点ではQEMU `mps2-an505`(Cortex-M33 x1、TrustZone対応ボード)上での
# 検証のみ。RP2350実機向けの差分はTODO.md「Phase 7」参照。

CC = arm-none-eabi-gcc
CPU_FLAGS = -mcpu=cortex-m33 -mthumb -mfloat-abi=hard -mfpu=fpv5-sp-d16
CFLAGS = $(CPU_FLAGS) -Os -std=gnu89 -Wall -Wextra -ffunction-sections -fdata-sections
LDFLAGS = $(CPU_FLAGS) -nostartfiles -T link.ld -Wl,--gc-sections \
	--specs=nano.specs --specs=nosys.specs

QEMU = qemu-system-arm
QEMU_MACHINE = mps2-an505

OBJS = startup.o uart.o syscalls.o main.o

all: hello.elf

%.o: %.c
	$(CC) -c -o $@ $(CFLAGS) $<

hello.elf: $(OBJS) link.ld
	$(CC) -o $@ $(OBJS) $(LDFLAGS)

# libc/malloc.cを-DPICOPERL_MALLOC_STATIC_HEAP付きでビルドし、静的配列を
# ヒープに使うベアメタル向けの経路が実際にCortex-M33上で動くことを確認。
LIBC_DIR = ../libc

# PICOPERL_HEAP_SIZEはこのQEMU検証用に64KBへ縮める(このMakefile独自の
# test-malloc.elf用リンカスクリプトの都合。RP2350実機での実際の
# ヒープサイズはPhase 7の残作業として別途見積もる。TODO.md参照)。
malloc_static.o: $(LIBC_DIR)/malloc.c $(LIBC_DIR)/picomalloc.h
	$(CC) -c -o $@ $(CFLAGS) -DPICOPERL_MALLOC_STATIC_HEAP -DPICOPERL_HEAP_SIZE='(64UL*1024UL)' -I$(LIBC_DIR) $(LIBC_DIR)/malloc.c

test-malloc.o: test-malloc.c uart.h $(LIBC_DIR)/picomalloc.h
	$(CC) -c -o $@ $(CFLAGS) -I$(LIBC_DIR) test-malloc.c

test-malloc.elf: startup.o uart.o syscalls.o malloc_static.o test-malloc.o link.ld
	$(CC) -o $@ startup.o uart.o syscalls.o malloc_static.o test-malloc.o $(LDFLAGS)

# QEMU上で数秒だけ走らせ、UART出力(printf経由・直接UART経由の両方)を
# 確認する。実機の割り込みや無限ループを止める手段がまだ無いため、
# `timeout`で強制終了する簡易テスト。
test: hello.elf test-malloc.elf
	@echo "--- expect 2 lines of UART output below ---"
	timeout 3 $(QEMU) -M $(QEMU_MACHINE) -cpu cortex-m33 \
		-kernel hello.elf -nographic -monitor none -serial stdio \
		| tee /tmp/picoperl-cm33-test.out; \
	grep -q "hello cortex-m33 from qemu (direct UART)" /tmp/picoperl-cm33-test.out && \
	grep -q "hello cortex-m33 from qemu (printf via newlib+_write)" /tmp/picoperl-cm33-test.out && \
	echo "ALL TESTS PASSED (hello)"
	@echo "--- malloc static-heap test ---"
	timeout 3 $(QEMU) -M $(QEMU_MACHINE) -cpu cortex-m33 \
		-kernel test-malloc.elf -nographic -monitor none -serial stdio \
		| tee /tmp/picoperl-cm33-malloc-test.out; \
	grep -q "^ALL TESTS PASSED$$" /tmp/picoperl-cm33-malloc-test.out && \
	echo "ALL TESTS PASSED (malloc)"

clean:
	rm -f $(OBJS) hello.elf malloc_static.o test-malloc.o test-malloc.elf

.PHONY: all clean test
