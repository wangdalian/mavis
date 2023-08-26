// todo: impl file system

    .section .rodata
    .global __hello_start
__hello_start:
     .incbin __wasm_path__
__hello_end:
    .globl __hello_size
__hello_size:
    .int __hello_end - __hello_start