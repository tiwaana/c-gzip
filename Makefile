CC=gcc
CFLAGS = -Werror -Wall


OBJ = gzip_header_parser.o

%.o: %.c
	$(CC) -g -c -o $@ $< $(CFLAGS)

gzip_header_parser: $(OBJ)
	gcc -o $@ $^ $(CFLAGS)

clean:
	rm -f *.o gzip_header_parser
