SRCDIR := core

LIB_CSOURCES += \
	$(SRCDIR)/arch.c \
	$(SRCDIR)/bus.c \
	$(SRCDIR)/core.c \
	$(SRCDIR)/device.c \
	$(SRCDIR)/elf.c \
	$(SRCDIR)/error.c \
	$(SRCDIR)/event.c \
	$(SRCDIR)/irq.c \
	$(SRCDIR)/list.c \
	$(SRCDIR)/lock.c \
	$(SRCDIR)/machine.c \
	$(SRCDIR)/mapper.c \
	$(SRCDIR)/mem.c \
	$(SRCDIR)/riscv/csr.c \
	$(SRCDIR)/riscv/dispatch.c \
	$(SRCDIR)/riscv/dispatch_rv32.c \
	$(SRCDIR)/riscv/dispatch_rv64.c \
	$(SRCDIR)/riscv/ex.c \
	$(SRCDIR)/riscv/regnames.c \
	$(SRCDIR)/riscv/riscv.c \
	$(SRCDIR)/sem.c \
	$(SRCDIR)/sym.c \

