CC=gcc
CFLAGS=-I/usr/local/igraph -L/usr/local/lib -ligraph -lgsl.

graham: graham.c
    $(CC) $(CFLAGS) -o graham graham.c