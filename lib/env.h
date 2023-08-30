#include <stdint.h>

/*
    env.h defines a list of built-in functions that the runtime has. 
    These can be used from the WASM binary.  
*/

__attribute__((
    __import_module__("env"),
    __import_name__("arch_serial_write"),
))
void arch_serial_write(char ch);

__attribute__((
    __import_module__("env"),
    __import_name__("arch_serial_read"),
))
int arch_serial_read(void);

__attribute__((
    __import_module__("env"),
    __import_name__("task_exit"),
))
__attribute__((noreturn))
void task_exit(int32_t code);