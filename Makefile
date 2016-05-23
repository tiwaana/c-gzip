CC=gcc
CFLAGS = -Werror -Wall


OBJ = at_gzip.o

%.o: %.c
	$(CC) -g -c -o $@ $< $(CFLAGS)

at_gzip: $(OBJ)
	gcc -o $@ $^ $(CFLAGS)

clean:
	rm -f *.o at_gzip
