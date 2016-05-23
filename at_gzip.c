#include <stdio.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#define GZIP_ID1	(0x1f)
#define GZIP_ID2	(0x8b)

#define EINVALIDID	(0x1)
#define ERESERVED	(0x2)
#define ECOMPRESSION	(0x3)
#define ENOTPRESENT	(0x10)
#define EINVALID	(0x100)
 
enum gzip_compression_method {
	e_cm0_reserved = 0,
	e_cm1_reserved,
	e_cm2_reserved,
	e_cm3_reserved,
	e_cm4_reserved,
	e_cm5_reserved,
	e_cm6_reserved,
	e_cm7_reserved,
	e_cm8_deflate,
	e_cm_invalid
};

#define	GZIP_FLAG_FTEXT		(1)
#define	GZIP_FLAG_FHCRC		(1<<1)
#define	GZIP_FLAG_FEXTRA	(1<<2)
#define	GZIP_FLAG_FNAME		(1<<3)
#define	GZIP_FLAG_FCOMMENT	(1<<4)
#define	GZIP_FLAG_RESERVED_5	(1<<5)
#define	GZIP_FLAG_RESERVED_6	(1<<6)
#define	GZIP_FLAG_RESERVED_7	(1<<7)

struct gzip_header_fixed {
	unsigned char gzip_id1;
	unsigned char gzip_id2;
	unsigned char gzip_cm;
	unsigned char gzip_flg;
	unsigned char gzip_mtime[4];
	unsigned char gzip_xfl;
	unsigned char gzip_os;
}; 

#define MAX_READ_BUF 4

/**
 * gzip_find_string_len() - find string length
 * @arg1:	file/stream handle
 *
 * Parse the stream by reading small chuck at a time and look
 * for end of string ie 0
 *
 * return:	returns the length of string, if EOS not found,
 * 		returns error.
 */
ssize_t gzip_find_string_len(int gzip_fd)
{
	char *buf = NULL;
	int complete = 0;
	int eof = 0;
	ssize_t str_len = 0;

	buf = malloc(MAX_READ_BUF);
	if (buf == NULL) {
		fprintf(stdout, "out of memory\n");
		return -ENOMEM;
	}
	
	while ((eof == 0) && (complete == 0)) {
		char *pos;
		ssize_t bytes_read = 0;

		errno = 0;
		bytes_read = read(gzip_fd, buf, MAX_READ_BUF);
		if ((bytes_read < 0) && (errno != EINTR)) {
			perror("read ");
			break;
		}
	
		pos = buf;
		if (bytes_read == 0) {
			eof = 1;
		}
		else {
			ssize_t len = 0;
			len = strnlen(buf, bytes_read);
			str_len += len;
			if (len != bytes_read) {
				complete = 1;
				break;
			}
		}
	}; 
	
	free(buf);
	if (eof == 1) {
		str_len = -EINVALID;
	}  
	return str_len;
}

/**
 * gzip_read_and_print_str() - copy string and print
 * @arg1:	file handle for gzip file
 * @arg2:	length of the string to read
 *
 * allocate a buffer and copy the the data from the file
 * of given size and print it. This function assumes that
 * file position is set correctly to read the string. 
 *
 * return:	0 on success and -1 on failure
 */
int gzip_read_and_print_str(int gzip_stream_fd, ssize_t str_len)
{
	char *str = malloc(str_len+1);
	
	if (str == NULL) {
		return -ENOMEM;
	}

	read(gzip_stream_fd, str, str_len+1);
	fprintf(stdout, "String : \"%s\" \n", str);

	free(str);

	return 0;
}

/**
 * check_gzip_header() - check gzip header for consistency
 * @arg1:	pointer to gzip fixed header struct
 *
 * This function checks if GZIP header is in compliance with 
 * RFC 1952 - GZIP file format specification version 4.3
 * 
 * return:	0 on success and error if header is not in 
 *		compliance with RFC. check error codes for
 *		specific reason.
 */
int check_gzip_header(struct gzip_header_fixed *pgzfh)
{
	int status = 0;
	
	if ((pgzfh->gzip_id1 != GZIP_ID1) 
		|| (pgzfh->gzip_id2 != GZIP_ID2)) {
			status = -EINVALIDID;
	}
	else if ((pgzfh->gzip_flg & GZIP_FLAG_RESERVED_5)
	        || (pgzfh->gzip_flg & GZIP_FLAG_RESERVED_6)
	        || (pgzfh->gzip_flg & GZIP_FLAG_RESERVED_7)) {
		status = -ERESERVED;
	} else if (pgzfh->gzip_cm != 8) {
		status = -ECOMPRESSION;
	}
	
	return status;
}

/** gzip_get_filename() - get filename of the gzipped file
 * @arg1:	handle of gzip fixed header struct
 * @arg2:	pointer to file stream, or file handle
 *
 * This function should check the flags and if NAME is set,
 * tries to extract the filename from the header. This function
 * assumes the stream pos is set at the start of the filename.
 *
 * return:	0 in success and error if can not extract filename
 *		or if filename is not present.
 */
int gzip_get_filename(struct gzip_header_fixed *pgzfh, int gzip_stream_fd)
{
	int status = 0;
	ssize_t filename_len = 0;
	off_t old_offset = 0;
	
	if ((pgzfh->gzip_flg & GZIP_FLAG_FNAME )) {
		old_offset = lseek(gzip_stream_fd, 0, SEEK_CUR);
		filename_len = gzip_find_string_len(gzip_stream_fd);
		if (filename_len > 0) {
			lseek(gzip_stream_fd, old_offset, SEEK_SET);
			gzip_read_and_print_str(gzip_stream_fd, filename_len);
		}
		else
			fprintf(stdout, "filename len exceeds filesize !! \n");
	}
	else {
		status = -ENOTPRESENT;
	}
	return status;
}
	 
/** gzip_get_comment() - get comment of the gzipped file
 * @arg1:	handle of gzip fixed header struct
 * @arg2:	pointer to file stream, or file handle
 *
 * This function should check the flags and if COMMENT is set,
 * it tries to extract the comment from the header. This function
 * assumes the stream pos is set at the start of comment.
 *
 * return:	0 in success and error if can not extract comment
 *		or if comment is not present.
 */
int gzip_get_comment(struct gzip_header_fixed *pgzfh, int gzip_stream_fd)
{
	int status = 0;
	ssize_t comment_len = 0;
	off_t old_offset = 0;
	
	if ((pgzfh->gzip_flg & GZIP_FLAG_FCOMMENT )) {
		old_offset = lseek(gzip_stream_fd, 0, SEEK_CUR);
		comment_len = gzip_find_string_len(gzip_stream_fd);
		if (comment_len > 0) {
			lseek(gzip_stream_fd, old_offset, SEEK_SET);
			gzip_read_and_print_str(gzip_stream_fd, comment_len);
		}
		else
			fprintf(stdout, "Comment length exceeds filesize !! \n");
	}
	else {
		status = -ENOTPRESENT;
	}
	return status;
}

int main(int argc, char ** argv)
{
	int gzip_fd = -1;
	struct gzip_header_fixed gzh;
	short gzip_xinfo_len;
	int bytes_read = 0;
	int status = 0;

	if (argc != 2) {
		fprintf(stdout, "Usage: %s filename\n", argv[0]);
		return EXIT_SUCCESS;
	}

	gzip_fd = open(argv[1], O_RDONLY);
	if (gzip_fd < 0) {
		fprintf(stderr, "Failed to open %s: ", argv[1]);
		perror("ERR");
		return EXIT_FAILURE;
	}

	bytes_read = read(gzip_fd, (void*)&gzh, sizeof(gzh));
	if (bytes_read != sizeof(gzh)) {
		perror("read failed: ");
		return EXIT_FAILURE;
	}

	status = check_gzip_header(&gzh);

	if (status != 0) {
		fprintf(stdout, "gzip header is not valid err: %d\n", status);
		return EXIT_FAILURE;
	}


#ifdef DEBUG		
	fprintf(stdout, "printing flags \n"
			" FTEXT: %s FEXTRA: %s FNAME: %s FCOMMENT %s \n",
			(gzh.gzip_flg & GZIP_FLAG_FTEXT ? "on" : "off"), 
			(gzh.gzip_flg & GZIP_FLAG_FEXTRA ? "on" : "off"),
			(gzh.gzip_flg & GZIP_FLAG_FNAME ? "on" : "off"),
			(gzh.gzip_flg & GZIP_FLAG_FCOMMENT ? "on" : "off"));
#endif	
	if (gzh.gzip_flg & GZIP_FLAG_FEXTRA) {
		bytes_read = read(gzip_fd, (void*)&gzip_xinfo_len, 2);
		if (bytes_read) {
			lseek(gzip_fd, (sizeof(gzh) + gzip_xinfo_len), SEEK_SET);
		}
	}
	else {
		lseek(gzip_fd, sizeof(gzh), SEEK_SET);
	}

	status = gzip_get_filename(&gzh, gzip_fd);
	if (status !=0 ) {
		if (status == -ENOTPRESENT) {
			fprintf(stdout, "No filename ! might be not from named file\n");
		}
		else {
			fprintf(stdout, "Cannot read filename\n");
		}
	}

	/* 
	 * BIG assumption here, we read upto the file name and according to gzip doc,
	 * comment, if present, should follow filename. so  I am not seeking .
	 */
	status = gzip_get_comment(&gzh, gzip_fd);
	if (status != 0) {
		if (status == -ENOTPRESENT) {
			fprintf(stdout, "No comments !\n");
		}
		else {
			fprintf(stdout, "Cannot read comments\n");
		}
	}
	
	close(gzip_fd);
	return EXIT_SUCCESS;
}
