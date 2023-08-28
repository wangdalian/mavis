(module
    (import "env" "exit" (func $exit (param i32)))
    (import "env" "puts" (func $puts (param i32)))

    (memory 1)
    (export "memory" (memory 0))

    (data (i32.const 0) "hello world\00")

    (func $main (result i32)
        (call $puts (i32.const 0))
        (i32.const 0)
    )
    
    (func (export "_start")
        call $main
        call $exit
    )
)