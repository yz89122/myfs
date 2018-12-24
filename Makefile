CC=gcc
CFLAGS=-Wall -std=c11

EXECUTABLE=myfs

$(EXECUTABLE): main.o myfs.o cmds.o utils.o
	$(CC) $(CFLAGS) main.o myfs.o cmds.o utils.o -o $(EXECUTABLE)
	strip $(EXECUTABLE)

myfs.o: myfs.c myfs.h
	$(CC) $(CFLAGS) -c myfs.c -o myfs.o

cmds.o: cmds.c cmds.h utils.h
	$(CC) $(CFLAGS) -c cmds.c

main.o: main.c myfs.h cmds.h utils.h
	$(CC) $(CFLAGS) -c main.c -o main.o

utils.o: utils.h utils.c
	$(CC) $(CFLAGS) -c utils.c -o utils.o

clean:
	rm *.o $(EXECUTABLE)
