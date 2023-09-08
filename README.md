# Mavis

## What is Mavis?
Mavis is an operating system I am developing for my own interest.
It implements WASM "userland", similar to [nebulet](https://github.com/nebulet/nebulet). 
The purpose of this project is to demonstrate the idea of "using WASM runtime as a kernel" and to explore the possibilities of WASM runtime and microkernel.

## Building with Docker & Running
You can build Mavis with Docker.
```
git clone git@github.com:wangdalian/mavis.git

# build compiler docker image
cd mavis/scripts
./build_compiler.sh

# build mavis
cd ..
./scripts/build_with_docker.sh

# install qemu
sudo apt install qemu-system-misc
make run
```

## Building & Running
Clone this repository and execute the "make" command. 
If you are lucky, the build will succeed ^^.

Execute the "make run" command to launch qemu.
If successful, you can see the shell(source code in servers/shell/main.c) as shown in the following image.

You can execute the "Hello World" program(source code in servers/hello/main.c) by executing the "hello" command.
The "exit" command will cause the kernel panic because there are no more tasks to execute.

![wasm-shell](https://github.com/RI5255/mavis/blob/images/wasm-shell.PNG)

## About pull requests
This project is completely for my personal interest, so it is always buggy and destructive changes are made. 
Therefore, I basically do not accept pull requests. 
Of course, extensions based on this project and discussions about this project are always welcome.
