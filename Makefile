# commands
CC := clang
LD := ld.lld
CP := cp
MKDIR := mkdir
RM := rm
QEMU := qemu-system-riscv32

top_dir := $(shell pwd)

# flags
CFLAGS := -std=c11 -O2 -g3 -Wall -Wextra --target=riscv32 -ffreestanding -nostdlib 
CFLAGS += -I$(top_dir)
LDFLAGS :=
QEMUFLAGS := -machine virt -bios default -nographic -serial mon:stdio --no-reboot

# variables
V ?=
BUILD_DIR ?= build
kernel_elf := $(BUILD_DIR)/kernel.elf

ifeq ($(V),)
.SILENT:
endif

# build kernel
.PHONY: build
build: $(kernel_elf)

build_dir := $(BUILD_DIR)/kernel
ldflags-y := -T$(BUILD_DIR)/kernel/kernel.ld
extra_deps := $(BUILD_DIR)/kernel/kernel.ld
objs-y := 
include kernel/build.mk

objs :=	$(BUILD_DIR)/libs/common.o \
		$(addprefix $(BUILD_DIR)/kernel/, $(objs-y))

$(kernel_elf): LDFLAGS := $(LDFLAGS) $(ldflags-y)
$(kernel_elf): OBJS := $(objs)
$(kernel_elf): $(objs) $(extra_deps)
	$(LD) $(LDFLAGS) -Map $(@:.elf=.map) -o $@ $(OBJS)

# build .o files
$(BUILD_DIR)/%.o: %.c
	$(MKDIR) -p $(@D)
	$(CC) $(CFLAGS) -c -o $@ $<

# linker script for kernel
$(BUILD_DIR)/kernel/kernel.ld: kernel/kernel.ld
	$(MKDIR) -p $(@D)
	$(CP) $< $@

# run qemu
.PHONY: run
run: build
	$(QEMU) $(QEMUFLAGS) -kernel $(kernel_elf)

# cleanup
.PHONY: clean
clean:
	$(RM) -rf $(BUILD_DIR)
