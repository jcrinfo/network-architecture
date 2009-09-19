CC = gcc
CFLAGS = -Wall
LDFLAGS = -L.
DEPFLAGS =

ftplib.o: ftplib.c ftplib.h
	$(CC) -c ftplib.c
ftp : ftp.c ftplib.o
	$(CC) -o ftp ftp.c ftplib.o
