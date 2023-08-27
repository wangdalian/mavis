#pragma once

/*
env.c defines built-in kernel functions that can be used from WASM binaries. 
For example, to use the env_exit function, import as follows

(import "env" "exit" (func $exit (param i32)))

See servers/hello/main.wat for more details.
*/

#include <stdint.h>

void env_exit(int32_t code);