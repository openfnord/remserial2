/*
 * ExampleBUS defines for parser example
 * Copyright (C) 2016  Ludwig Jaffe, ludwig.jaffe@gmail.com
 *
 *
 */
 

//#######################
//## CHARS in protocol ##
//#######################

#define DLE 0x10
#define SOF 0x1F
#define ACK 0x06
#define SAD 0x02
#define EOF 0x17

//#########################
//## OFFSETS in protocol ##
//#########################

//HEADER
#define BYTEPOSITION_START_DLE      0  // Relative to plain response
#define BYTEPOSITION_SOF 	    1  // Relative to plain response
#define BYTEPOSITION_CONTROL        2  // Relative to plain response
#define BYTEPOSITION_PACKETNUMBER   3  // Relative to plain response
#define BYTEPOSITION_DATALEN        4  // Relative to plain response
#define BYTEPOSITION_FIRST_DATA     5  // Relative to plain response

#define BYTEPOSITION_MAX_DATA       121 + BYTEPOSITION_FIRST_DATA  // Relative to plain response
    
//FOOTER 
#define FOOTER_LENGTH			3   //	  3= CRC + DLE + EOF
#define BYTEPOSITION_EOF 	        -1  // Relative to end of plain response
#define BYTEPOSITION_END_DLE       	-2  // Relative to end of plain response
#define BYTEPOSITION_CRC 	        -3  // Relative to end of plain response
#define BYTEPOSITION_LAST_DATA      	-4  // Relative to end of plain response
 
//SIZES
#define MINIMAL_RESPONSE_LENGTH		BYTEPOSITION_FIRST_DATA + FOOTER_LENGTH



