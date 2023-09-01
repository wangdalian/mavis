// todo: impl file system
    .section .data, "",@
    .global __hello_start
    .size __hello_start, 0
__hello_start:
     .incbin __wasm_path__
__hello_end:
    .size __hello_end, 0
    .globl __hello_size
    .size __hello_size, 4
__hello_size:
    .int __hello_end - __hello_start