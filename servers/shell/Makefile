CC 				:= clang
WASI_SDK_CLANG	:= $(HOME)/wasi-sdk-20.0/bin/clang
MKDIR 			?= mkdir 
build_dir 		?=
lib 			?=
include 		?=

wasm 			:= $(build_dir)/main.wasm
obj 			:= $(build_dir)/main.o

.PHONY: build
build: $(wasm) $(obj)

$(wasm): main.c $(lib)
	$(MKDIR) -p $(@D)
	$(WASI_SDK_CLANG) -nostdlib -I$(include) $^ -o $@

$(obj): $(wasm)
$(obj): main.S
	$(MKDIR) -p $(@D)
	$(CC) -D__wasm_path__='"$(wasm)"' --target=riscv32 -c -o $@ $<