SRCDIR := core

LIB_CSOURCES += \
	$(SRCDIR)/arch.c \
	$(SRCDIR)/bus.c \
	$(SRCDIR)/cache.c \
	$(SRCDIR)/chrono.c \
	$(SRCDIR)/core.c \
	$(SRCDIR)/device.c \
	$(SRCDIR)/elf.c \
	$(SRCDIR)/engine.c \
	$(SRCDIR)/error.c \
	$(SRCDIR)/ex.c \
	$(SRCDIR)/host.c \
	$(SRCDIR)/io.c \
	$(SRCDIR)/irq.c \
	$(SRCDIR)/list.c \
	$(SRCDIR)/lock.c \
	$(SRCDIR)/machine.c \
	$(SRCDIR)/mapper.c \
	$(SRCDIR)/mem.c \
	$(SRCDIR)/regview.c \
	$(SRCDIR)/riscv/csr.c \
	$(SRCDIR)/riscv/dispatch.c \
	$(SRCDIR)/riscv/dispatch_fp32.c \
	$(SRCDIR)/riscv/dispatch_fp64.c \
	$(SRCDIR)/riscv/dispatch_rv32.c \
	$(SRCDIR)/riscv/dispatch_rv64.c \
	$(SRCDIR)/riscv/regnames.c \
	$(SRCDIR)/riscv/riscv.c \
	$(SRCDIR)/riscv/rvex.c \
	$(SRCDIR)/sem.c \
	$(SRCDIR)/sym.c \
	$(SRCDIR)/worker.c \

