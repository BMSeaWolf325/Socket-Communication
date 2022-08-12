all: main.o communicate.o
	gcc main.o communicate.o -pthread -o final

main.o: main.c communicate.h
	gcc -c main.c

communicate.o: communicate.c communicate.h
	gcc -c communicate.c

clean:
	rm -f *.o final