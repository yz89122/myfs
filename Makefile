CC=gcc
ARGS="-Wall"

a.out: main.o myfs.o commands.o utils.o
	$(CC) $(ARGS) main.o myfs.o commands.o utils.o
	strip a.out

myfs.o: myfs.c myfs.h
	$(CC) $(ARGS) -c myfs.c -o myfs.o

commands.o: commands.c commands.h utils.h
	$(CC) $(ARGS) -c commands.c

main.o: main.c myfs.h commands.h utils.h
	$(CC) $(ARGS) -c main.c -o main.o

utils.o: utils.h utils.c
	$(CC) $(ARGS) -c utils.c -o utils.o

