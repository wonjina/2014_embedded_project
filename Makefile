CROSS = arm-linux-gnueabihf-
CC = $(CROSS)gcc
CFLAGS = -Wall -O2 -Wunused-but-set-variable
OBJS = fbtest.o
all: fbtest.o 
	$(CC) $(CFLAGS) -o fbtest -pthread $(OBJS) 
clean:
	rm -f *.o fbtest
