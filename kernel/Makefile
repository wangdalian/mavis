# commands
CC := clang
LD := ld.lld
MKDIR := mkdir
CP := cp
RM := rm
OBJCOPY  := llvm-objcopy
QEMU := qemu-system-riscv32

# flags
CFLAGS :=-std=c11 -O2 -g3 -Wall -Wextra --target=riscv32 -ffreestanding -nostdlib
QEMUFLAGS := -machine virt -bios default -nographic -serial mon:stdio --no-reboot

# build settings
BUILD_DIR ?= build

kernel_elf = $(BUILD_DIR)/kernel.elf

.PHONY: build
build: $(kernel_elf)

# build kernel
objs := $(addprefix $(BUILD_DIR)/, kernel.o common.o)
linker_script := $(BUILD_DIR)/kernel.ld
$(kernel_elf): OBJS := $(objs)
$(kernel_elf): LDFLAGS := -T$(linker_script)

$(kernel_elf): $(objs) $(linker_script)
	$(LD) $(LDFLAGS) -Map $(@:.elf=.map) -o $@ $(OBJS)

$(BUILD_DIR)/%.o: %.c
	$(MKDIR) -p $(@D)
	$(CC) $(CFLAGS) -c -o $@ $<

# linker script for kernel
$(BUILD_DIR)/kernel.ld: kernel.ld
	$(MKDIR) -p $(@D)
	$(CP) $< $@

.PHONY: run
run: build
	$(QEMU) $(QEMUFLAGS) -kernel $(kernel_elf)

.PHONY: clean
clean:
	$(RM) -rf $(BUILD_DIR)