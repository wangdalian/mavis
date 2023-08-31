// todo: impl file system

    .section .rodata
    .global __shell_start
__shell_start:
     .incbin __wasm_path__
__shell_end:
    .globl __shell_size
__shell_size:
    .int __shell_end - __shell_start