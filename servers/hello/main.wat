(module
    (import "wasi_unstable" "fd_write" (func $fd_write (param i32 i32 i32 i32) (result i32)))
    (import "env" "exit" (func $exit (param i32)))

    (memory 1)
    (export "memory" (memory 0))

    (data (i32.const 8) "hello world\n")

    (func $main (result i32)
        (i32.store (i32.const 0) (i32.const 8))
        (i32.store (i32.const 4) (i32.const 12))

        (call $fd_write
            (i32.const 1)
            (i32.const 0)
            (i32.const 1)
            (i32.const 20)
        )
    )
    
    (func (export "_start")
        call $main
        call $exit
    )
)