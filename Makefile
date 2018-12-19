CC=gcc
ARGS="-Wall"

a.out: main.o myfs.o
	$(CC) $(ARGS) main.o myfs.o 
	strip a.out

myfs.o: myfs.c myfs.h
	$(CC) $(ARGS) -c myfs.c -o myfs.o

main.o: main.c myfs.h
	$(CC) $(ARGS) -c main.c -o main.o

