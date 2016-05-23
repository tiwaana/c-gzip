CC=gcc
OBJ = at_gzip.o 

%.o: %.c
	$(CC) -c -o $@ $< $(CFLAGS)

at_gzip: $(OBJ)
	gcc -o $@ $^ $(CFLAGS)

clean:
	rm -f *.o at_gzip
