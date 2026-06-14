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
	$(SRCDIR)/riscv/decode.c \
	$(SRCDIR)/riscv/dispatch.c \
	$(SRCDIR)/riscv/regnames.c \
	$(SRCDIR)/riscv/riscv.c \
	$(SRCDIR)/riscv/rvex.c \
	$(SRCDIR)/sem.c \
	$(SRCDIR)/slac.c \
	$(SRCDIR)/slac4.c \
	$(SRCDIR)/slac8.c \
	$(SRCDIR)/sym.c \
	$(SRCDIR)/worker.c \

