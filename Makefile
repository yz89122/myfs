CC=gcc
ARGS="-Wall"

a.out: main.o myfs.o cmds.o utils.o
	$(CC) $(ARGS) main.o myfs.o cmds.o utils.o -o a.out
	strip a.out

myfs.o: myfs.c myfs.h
	$(CC) $(ARGS) -c myfs.c -o myfs.o

cmds.o: cmds.c cmds.h utils.h
	$(CC) $(ARGS) -c cmds.c

main.o: main.c myfs.h cmds.h utils.h
	$(CC) $(ARGS) -c main.c -o main.o

utils.o: utils.h utils.c
	$(CC) $(ARGS) -c utils.c -o utils.o

