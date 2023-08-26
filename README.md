# Mavis

## What is Mavis?
Mavis is an operating system I am developing for my own interest.
It implements WASM "userland", similar to [nebulet](https://github.com/nebulet/nebulet). 
The purpose of this project is to demonstrate the idea of "using WASM runtime as kernel" and to explore the possibilities of WASM runtime and microkernel.

## Building & Running
Clone this repository and execute the "make" command. 
If you are lucky, the build will succeed ^^.

Execute the "make run" command to launch qemu.
If successful, you can see the shell as shown in the following image.

![shell](https://github.com/RI5255/mavis/blob/images/shell.PNG)


If you execute the "hello" command, a WASM binary that outputs "hello world" will be executed. 
The source code is in servers/hello/main.wat.
If you execute the "exit" command, shell terminates and the kernel panics because there are no more tasks to execute.

![hello](https://github.com/RI5255/mavis/blob/images/hello.PNG)

## About pull requests
This project is completely for my personal interest, so it is always buggy and destructive changes are made. 
Therefore, I basically do not accept pull requests. 
Of course, extensions based on this project and discussions about this project are always welcome.
