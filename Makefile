# commands
CC := clang
WASI_SDK_CLANG := $(HOME)/wasi-sdk-20.0/bin/clang
LD := ld.lld
MKDIR := mkdir
CP := cp
RM := rm
OBJCOPY  := llvm-objcopy
QEMU := qemu-system-riscv32
WAT2WASM := wat2wasm

# build settings
ARCH := riscv32
BUILD_DIR ?= build

kernel_elf = $(BUILD_DIR)/kernel.elf

# flags
CFLAGS :=-std=c11 -O2 -g3 -Wall -Wextra --target=riscv32 -ffreestanding -nostdlib
TOP_DIR := $(shell pwd)
CFLAGS += -I$(TOP_DIR)
CFLAGS += -Ikernel/$(ARCH)/include
QEMUFLAGS := -machine virt -bios none -nographic -serial mon:stdio --no-reboot

# servers
all_servers := $(notdir $(patsubst %/main.c, %, $(wildcard servers/*/main.c)))

.PHONY: build
build: servers $(kernel_elf)

arch_obj := $(BUILD_DIR)/kernel/$(ARCH).o

# object files required to build the kernel
objs := $(addprefix $(BUILD_DIR)/kernel/, kernel.o common.o buffer.o list.o module.o vm.o task.o env.o memory.o) \
		$(foreach s, $(all_servers), $(addprefix $(BUILD_DIR)/servers/$(s)/, main.o)) \
		$(arch_obj)

# rules for building kernel
linker_script := $(BUILD_DIR)/kernel/kernel.ld
$(kernel_elf): OBJS := $(objs)
$(kernel_elf): LDFLAGS := -T$(linker_script)
$(kernel_elf): $(objs) $(linker_script)
	$(LD) $(LDFLAGS) -Map $(@:.elf=.map) -o $@ $(OBJS)

$(arch_obj): $(addprefix $(BUILD_DIR)/kernel/$(ARCH)/, boot.o common.o task.o trap.o uart.o)
	$(MKDIR) -p $(@D)
	$(LD) -r -o $@ $^ 

$(BUILD_DIR)/%.o: %.c
	$(MKDIR) -p $(@D)
	$(CC) $(CFLAGS) -c -o $@ $<

# linker script for kernel
$(BUILD_DIR)/kernel/kernel.ld: kernel/kernel.ld
	$(MKDIR) -p $(@D)
	$(CP) $< $@

# rulues for building servers
define build_server
	$(eval build_dir := $(BUILD_DIR)/servers/$(1))
	$(eval src := servers/$(1)/main.c)
	$(eval libs := lib/common.c)
	$(eval asm := servers/$(1)/main.S)
	$(eval target := $(build_dir)/main.o)
	$(MKDIR) -p $(build_dir)
	$(WASI_SDK_CLANG) $(src) $(libs) -nostdlib -I$(TOP_DIR) -o $(build_dir)/main.wasm
	$(CC) -D__wasm_path__='"$(build_dir)/main.wasm"' --target=riscv32 -c -o $(target) $(asm)

endef

.PHONY: servers
servers:
	$(foreach s, $(all_servers), $(call build_server,$(s)))

# run qemu
.PHONY: run
run: build
	$(QEMU) $(QEMUFLAGS) -kernel $(kernel_elf)

# cleanup
.PHONY: clean
clean:
	$(RM) -rf $(BUILD_DIR)