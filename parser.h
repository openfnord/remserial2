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


/*TODO:
We need to make a parser interface, a good one.
So one would be a function prototype that has a zero implementation, a 
zerro parser and a pointer that points to it by default if there is no
parser.
But we could also have a parser that is more flexible!
Lets make a tcp socket where we put the traffic to be parsed in,
and get the traffic to be parsed out. 
And then, we put the traffic out at our tcp socket to the client.

You see, we dont need a parser, we just need a parser that talks tcp
on both sides!!
So we connect remserial to one side of the parser and the client to the
other side of the parser very flexible using tcp/ip.

So dont write a parser?



