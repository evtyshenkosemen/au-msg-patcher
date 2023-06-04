#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <memory.h>
#include <stdbool.h>
#include "msg_parser.h"
#include "crc32.h"

/**
    \file
    \brief Message parser library
	 You need to write a program in C language, which will process messages:
	 - read pairs of message and mask from file (see Message input format)
	 - check CRC-32 for data field (see Important notes)
	 - add zero padding to the data bytes if it is not a multiple of 4
	 - divide data bytes in tetrads and apply mask to even tetrads
	 - calculate CRC-32 for new data bytes
	 - save results to output file (see Output information format)
 
	Message input format:
	|-------------------------------------------|
	|  0   |    1   |   2...253   |  254...257  |
	|-------------------------------------------|
	| Type | Length |          Payload          |
	|-------------------------------------------| 
	|               |   data      |    CRC-32   |
	|-------------------------------------------| 

	- message leaded by keyword `mess=`

	 Mask input format
		- 32 bits value 
		- mask leaded by keyword `mask=`
	 
	Output information format (for each message):
		- message type
		- initial message length
		- initial message data bytes
		- initial CRC-32- modified message length
		- modified message data bytes with mask
		- modified CRC-32

 	Important notes:
		- input file name: "data_in.txt"
		- output file name: "data_out.txt"
		- input and output files stored in the same directory as the program itself
		- if output file exist, it have to be updated with new information
		- all errors have to be saved in output file
		- all data in input file stored in ASCII hex format
		- all data stored in ASCII hex format
		- CRC-32 polynome 0x04C11DB7, initial value 0xFFFFFFFF
*/

char mess_header[]    = MESS_HEADER;
char mask_header[]    = MASK_HEADER;
  
// Maximum range of bytes
int read_input()
{
	bool mess_hdr_found = false;
	bool mask_hdr_found = false;

	uint8_t mess_hdr_next = 0;
	uint8_t mask_hdr_next = 0;

	union mess_UNT mess;
	uint32_t       mask;

    FILE* fptr1;
    char c;
    int i;
  
    fptr1 = fopen("./"IN_FILE_NAME, "r");
  
    if (fptr1 == NULL) {
		printf("No file %s\n", "./"IN_FILE_NAME);
        return 1;
    }
  
    for (i = 0; c != EOF; i++) {

        // Get the character
        c = fgetc(fptr1);

		/* waiting for headers */
		if(c == mess_header[mess_hdr_next])
			mess_hdr_next++;
		else
			mess_hdr_next=0;
		
		
		if(c == mask_header[mask_hdr_next])
			mask_hdr_next++;
		else
			mask_hdr_next=0;
		
		/* catch header found */
		if(mess_hdr_next == sizeof(mess_header)-1){ /* exclude '\0' with -1 */ 
			/* we found "mess=" header */
			if (mess_hdr_found){
				fclose(fptr1);
				return ERR_MESS_DUPLICATE_FOUND;
			}

			parse_mess(fptr1, &mess);

			print_mess(&mess);
			mess_hdr_found = true;
			mess_hdr_next=0;
		}

		if(mask_hdr_next == sizeof(mask_header)-1){ /* exclude '\0' with -1 */ 
			/* we found "mask=" header */
			if (mask_hdr_found){
				fclose(fptr1);
				return ERR_MASK_DUPLICATE_FOUND;
			}

			parse_mask(fptr1, &mask);
			printf("mask=0x%x\n",mask);

			mask_hdr_found = true;
			mask_hdr_next=0;
		}

		if(mask_hdr_found && mess_hdr_found){
			mask_hdr_found = false;
			mess_hdr_found = false;
			printf("==============\n");
			//process_mess_mask_pair();
		}
    }
  
    fclose(fptr1);
  
    return 0;
}

/** 

	/params

*/

/**
    \brief 
        Parse mess header 

    \details

    \param[in] file Pointer to the file with pointer on msg start

    \return 
		ERR_UNEXCEPTED_END_OF_FILE - if read falls
		0 - on success
*/

int parse_mess_header(FILE* mess_PTR, union mess_UNT * mess){
	char buf[MAX(CHARSET_ONE_BYTE_SIZE*sizeof(mess->per_byte.header.type), CHARSET_ONE_BYTE_SIZE*sizeof(mess->per_byte.header.length))+1]={'\0'};

	uint8_t bytes_read; /** \todo add a preprocessor err if you want to read more than uint8_t allows to address */
    uint8_t bytes_to_read; 
	/* parse header->type */
	bytes_to_read = CHARSET_ONE_BYTE_SIZE*sizeof(mess->per_byte.header.type);
	bytes_read = fread(buf, 1, bytes_to_read, mess_PTR);
	if(bytes_read != bytes_to_read)
		return ERR_UNEXCEPTED_END_OF_FILE;
	mess->per_byte.header.type = strtol(buf, NULL, 16); /* parse int from chars */ /** \todo add errcheck here */

	/* parse header->type */
	bytes_to_read = CHARSET_ONE_BYTE_SIZE*sizeof(mess->per_byte.header.length);
	bytes_read = fread(buf, 1, bytes_to_read, mess_PTR);
	if(bytes_read != bytes_to_read)
		return ERR_UNEXCEPTED_END_OF_FILE;
	mess->per_byte.header.length = strtol(buf, NULL, 16); /* parse int from chars */ /** \todo add errcheck here */

	return 0;
}

/**
    \brief 
        Parse mess data 

    \details

    \param[in] file Pointer to the file with pointer on data start

    \return 
		ERR_UNEXCEPTED_END_OF_FILE - if read falls
		0 - on success
*/

int parse_mess_data(FILE* mess_PTR, union mess_UNT * mess){
	
	int          i;
	uint8_t      bytes_read; /** \todo add a preprocessor err if you want to read more than uint8_t/uint16_t allows to address */
	uint16_t     bytes_to_skip;
	char         buf[CHARSET_ONE_BYTE_SIZE*sizeof(uint8_t)+1]={'\0'};

    /* set high bytes of STC to 0x00 */
	bytes_to_skip = sizeof(mess->per_byte.data) - mess->per_byte.header.length;
	memset(mess->per_byte.data, 0x00, bytes_to_skip);

	/* parse payload */
	for(i=bytes_to_skip; i < sizeof(mess->per_byte.data) ;i++)
	{
		
		bytes_read = fread(buf, 1, CHARSET_ONE_BYTE_SIZE, mess_PTR);
		if(bytes_read != CHARSET_ONE_BYTE_SIZE)
			return ERR_UNEXCEPTED_END_OF_FILE;

		mess->per_byte.data[i] = strtol(buf, NULL, 16);
	}

	return 0;
}

/**
    \brief 
        Parse mess crc32 field

    \details

    \param[in] file Pointer to the file with pointer on crc32 start

    \return 
		ERR_UNEXCEPTED_END_OF_FILE - if read falls
		0 - on success
*/
int parse_mess_crc(FILE* mess_PTR, union mess_UNT * mess){
	char buf[CHARSET_ONE_BYTE_SIZE*sizeof(mess->per_byte.crc32)+1]={'\0'};

	uint8_t bytes_read; /** \todo add a preprocessor err if you want to read more than uint8_t allows to address */
    uint8_t bytes_to_read; 
	/* parse crc */
	bytes_to_read = CHARSET_ONE_BYTE_SIZE*sizeof(mess->per_byte.crc32);
	bytes_read = fread(buf, 1, bytes_to_read, mess_PTR);
	if(bytes_read != bytes_to_read)
		return ERR_UNEXCEPTED_END_OF_FILE;
	mess->per_byte.crc32 = strtol(buf, NULL, 16); /* parse int from chars */ /** \todo add errcheck here */

	return 0;
}


int parse_mess(FILE* mess_PTR, union mess_UNT * mess){
	int error;

	error = parse_mess_header(mess_PTR, mess);
	if(error)
		return error;

	error = parse_mess_data(mess_PTR, mess);
	if(error)
		return error;

	error = parse_mess_crc(mess_PTR, mess);
	if(error)
		return error;

	return 0;
}

crc32_t check_sum_mess(union mess_UNT * mess){
	crc32_t mess_crc = 0;
	crc32_recalculate_fake(mess, sizeof(mess->bytes)-sizeof(mess->per_byte.crc32), &mess_crc); /* free bytes problem using real CRC*/

	return mess_crc;
}

int parse_mask(FILE* mess_PTR, uint32_t *mask){
	int error;
	char buf[CHARSET_ONE_BYTE_SIZE*sizeof(uint32_t)+1]={'\0'};

	uint8_t bytes_read; /** \todo add a preprocessor err if you want to read more than uint8_t allows to address */
    uint8_t bytes_to_read; 
	/* parse crc */
	bytes_to_read = CHARSET_ONE_BYTE_SIZE*sizeof(uint32_t);
	bytes_read = fread(buf, 1, bytes_to_read, mess_PTR);
	if(bytes_read != bytes_to_read)
		return ERR_UNEXCEPTED_END_OF_FILE;
	*mask = strtol(buf, NULL, 16); /* parse int from chars */ /** \todo add errcheck here */

	return 0;
}

void print_mess(union mess_UNT * mess){
	printf("Type: 0x%x\n", mess->per_byte.header.type);
	printf("Leng:0x%x\n", mess->per_byte.header.length);

	for(int i=(sizeof(mess->per_byte.data) - mess->per_byte.header.length); i < sizeof(mess->per_byte.data) ;i++)
	{
		printf("   0x%x\n", mess->per_byte.data[i]);
	}
	printf("CRC: 0x%x status:%s \n", mess->per_byte.crc32, (check_sum_mess(mess) == mess->per_byte.crc32) ? "CORRECT" : "WRONG");

}

int process_mess_mask_pair(union mess_UNT * mess, uint32_t *mask){
	
	return 0;
}