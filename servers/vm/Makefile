CC = clang
CFGLAGS = -Wall -Wextra
OBJS = buffer.o main.o node.o list.o

.PHONY: build
build: vm.elf

vm.elf: $(OBJS)

	$(CC) $(CFGLAGS) -o $@ $^

%.o: %.c
	$(CC) $(CFGLAGS) -c -o $@ $<

.PHONY: clean
clean:
	rm -f vm.elf *.o