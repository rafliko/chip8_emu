CC = gcc
CFLAGS = -Wall

compile:
	$(CC) $(CFLAGS) frontend.c backend.c -o c8
