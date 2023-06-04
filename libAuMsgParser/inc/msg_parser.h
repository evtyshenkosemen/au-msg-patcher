#ifndef ALLOCATOR_H
#define ALLOCATOR_H

#include <stdint.h>
#include "autoconf.h"


#define IN_FILE_NAME     "data_in.txt"
#define OUT_FILE_NAME    "data_out.txt"
#define MESS_HEADER      "mess="
#define MASK_HEADER      "mask="

#define CHARSET_ONE_BYTE_SIZE ASCII_ONE_BYTE_SIZE

#define ASCII_ONE_BYTE_SIZE 2

/*
    Input message format:
    |------------------------------------------------------------------|
	|  0   |    1   | 2...253 (252 bytes max)   |  254...257 (4bytes)  |
	|------------------------------------------------------------------|
	| Type | Length |                     Payload                      |
	|------------------------------------------------------------------| 
	|               |       data                         |   CRC-32    |
	|------------------------------------------------------------------| 

*/

typedef struct __attribute__((__packed__)) _msg_h
{
    uint8_t    type;
    uint8_t    length;
}msg_hdr_STC;
typedef struct __attribute__((__packed__)) _msg_t
{
    msg_hdr_STC    header;
    uint32_t       tetrad[63]; /* 252 / 4 = 63 */
    uint16_t       crc32;  
}msg_tetrads_STC;

typedef struct __attribute__((__packed__)) _msg_b
{
    msg_hdr_STC    header;
    uint8_t        data[252];
    uint16_t       crc32;
} msg_bytes_STC;

union mess_UNT {
    msg_tetrads_STC    per_tetrad;
    msg_bytes_STC      per_byte;
};

/* public functions */

int read_input();
int parse_mess(FILE* mess_PTR, union mess_UNT * mess);
int parse_mess_header(FILE* mess_PTR, union mess_UNT * mess);
int parse_mess_data(FILE* mess_PTR, union mess_UNT * mess);
void print_mess(union mess_UNT * mess);


#define MAX(a,b) (((a)>(b))?(a):(b))

/* errors */

#define ERR_MESS_DUPLICATE_FOUND -1
#define ERR_MASK_DUPLICATE_FOUND -2
#define ERR_UNEXCEPTED_END_OF_FILE -3

#endif //ALLOCATOR_H