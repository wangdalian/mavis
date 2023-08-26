    .section .rodata
    .global __hello_start
__hello_start:
     .incbin "hello.wasm"
__hello_end:
    .global __hello_size
__hello_size:
        .int __hello_end - __hello_start