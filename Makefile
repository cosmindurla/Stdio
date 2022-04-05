CC = gcc
C_FLAGS = -Wall -fPIC -g
LIB_FLAGS = -shared

.PHONY: build clean

build: libso_stdio.so

libso_stdio.so: stdio.c
	$(CC) $(C_FLAGS) -c stdio.c
	$(CC) $(LIB_FLAGS) stdio.o -o libso_stdio.so

clean: stdio.o libso_stdio.so
	rm stdio.o libso_stdio.so