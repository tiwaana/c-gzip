# c-gzip

Please write a small C program that will parse out and print the optional name and comment fields from the header in a gzip file, which is well documented in RFC-1952: https://tools.ietf.org/html/rfc1952 

Note that you don't need to check the optional CRC16 checksum for the gzip header, nor do you need to actually decompress the DEFLATE playload in a gzip file. Please do not assume that the header in the gzip file being processed is well-formed. 

Be careful to correctly handle error conditions. Keep in mind that such code would need to gracefully and securely handle maliciously crafted input. 

TODO:
This is not very good in handling maliciously crafted input, need to learn more about character sets
