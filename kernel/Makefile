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
objs := $(addprefix $(BUILD_DIR)/kernel/, kernel.o common.o buffer.o list.o module.o vm.o) \
		$(addprefix $(BUILD_DIR)/servers/, hello.o)

linker_script := $(BUILD_DIR)/kernel/kernel.ld
$(kernel_elf): OBJS := $(objs)
$(kernel_elf): LDFLAGS := -T$(linker_script)

$(kernel_elf): $(objs) $(servers) $(linker_script)
	$(LD) $(LDFLAGS) -Map $(@:.elf=.map) -o $@ $(OBJS)

$(BUILD_DIR)/kernel/%.o: %.c
	$(MKDIR) -p $(@D)
	$(CC) $(CFLAGS) -c -o $@ $<

$(BUILD_DIR)/servers/hello.o: hello.s
	$(MKDIR) -p $(@D)
	$(CC) --target=riscv32 -c -o $@ $<

# linker script for kernel
$(BUILD_DIR)/kernel/kernel.ld: kernel.ld
	$(MKDIR) -p $(@D)
	$(CP) $< $@

.PHONY: run
run: build
	$(QEMU) $(QEMUFLAGS) -kernel $(kernel_elf)

.PHONY: clean
clean:
	$(RM) -rf $(BUILD_DIR)