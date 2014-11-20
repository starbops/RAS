all: main.o cmd.o pipen.o
	gcc main.o cmd.o pipen.o -o server
main.o: main.c cmd.h pipen.h
	gcc -I . -c main.c
cmd.o: cmd.c cmd.h
	gcc -I . -c cmd.c
pipen.o: pipen.c pipen.h
	gcc -I . -c pipen.c
clean:
	rm -rf *.o
	rm server
