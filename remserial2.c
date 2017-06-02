/*
 * remserial
 * Copyright (C) 2000  Paul Davis, pdavis@lpccomp.bc.ca
 * Copyright (C) 2015  Ludwig Jaffe, ludwig.jaffe@gmail.com
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 *
 * This program acts as a bridge either between a socket(2) and a
 * serial/parallel port or between a socket and a pseudo-tty.
 *
 * Extended 2015 by Ludwig Jaffe
 * contains code from hexdump example of  
 * simplestcodings.blogspot.com/2010/08/hex-dump-file-in-c.html
 *
 */
#include "parser.h" 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <syslog.h>
#include <signal.h>
#include <errno.h>
#include <sys/time.h> //print time stamps
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#define DEVBUFSIZE 512
#define HEX_OFFSET    1
#define ASCII_OFFSET 30 //51
#define NUM_CHARS    8  //16 is too much for both directions (tx,rx) so we choose 8 here!
#define RIGHTLEFTOFFSET   39  //offset between tx side and rx side



struct sockaddr_in addr,remoteaddr;
int sockfd = -1;
int port = 23000;
int debug=0;
int timestamps=0;
int loud=1; //if >0 then we are chatty to the console, option q -> loud=0
int devfd;
int *remotefd;
char *machinename = NULL;
char *sttyparms = NULL;
static char *sdevname = NULL;
char *linkname = NULL;
int isdaemon = 0;
int hexdump = 0;
fd_set fdsread,fdsreaduse;
struct hostent *remotehost;
extern char* ptsname(int fd);
int curConnects = 0;
double timestartms = 0;	//holds time at program start to calc time difference


void sighandler(int sig);
int connect_to(struct sockaddr_in *addr);
void usage(char *progname);
void synopsis(char *progname);
void version(char *progname);
void examples(char *progname);
void link_slave(int fd);

void dumpbuf(int count, char * buffer, int left_right); 
void print_timestamp();


/* Clear the display line.  */
void   clear_line (char *line, int size);
/* Put a character (in hex format
             * into the display line. */
char * hex   (char *position, int c); 
/* Put a character (in ASCII format
             * into the display line. */
char * ascii (char *position, int c);
//for dumpbuf  (screen positions are global, as we need  to call left pane and right pane)

char *hex_offset;     /* Position of the next character
                                                 * in Hex     */
char *ascii_offset;      /* Position of the next character
                                                       * in ASCII.      */
char line[81];        /* O/P line.      */



int main(int argc, char *argv[])
{
	int result;
	extern char *optarg;
	extern int optind;
	int maxfd = -1;
	char devbuf[DEVBUFSIZE];
	int devbytes;
	int remoteaddrlen;
	int c;
	int waitlogged = 0;
	int maxConnects = 1;
	int writeonly = 0;
        int parsed_args =0;
	register int i;
	struct timeval  tv;
	double time_in_ms=0;
	
	gettimeofday(&tv, NULL);
	timestartms = ((tv.tv_sec) * 1000 + (tv.tv_usec) / 1000) ; // convert tv_sec & tv_usec to ms

		while ( (c=getopt(argc,argv,"del:m:p:r:st:uvwx:Hh")) != -1 )
			switch (c) {
			case 'd':
                		parsed_args++;
				isdaemon = 1;
				printf (" deamon ");
				break;

			case 'e':
                		parsed_args++;
				examples(argv[0]);
				exit(1);
				break;
			case 'h':
                		parsed_args++;
				hexdump = 1;
				printf (" hexdump ");
				break;
			case 'H':
                		parsed_args++;
				hexdump = 2;
				printf (" hexdump ");
				break;
			case 'l':
                		parsed_args++;
				linkname = optarg;
				printf ("link to slave pty: %s",linkname);
				break;
			case 'm':
                		parsed_args++;
				maxConnects = atoi(optarg);
				printf ("max connections: %d",maxConnects);
				break;
			case 'p':
                		parsed_args++;
				port = atoi(optarg);
				printf ("port: %d",port);
				break;

			case 'q':
                		parsed_args++;
				loud = 0;
				break;
			case 'r':
                		parsed_args++;
				machinename = optarg;
				printf ("machine name: %s",machinename);
				break;
			case 's':
                		parsed_args++;
				sttyparms = optarg;
				printf ("stty params: %s",sttyparms);
				break;
			case 't':
                		parsed_args++;
				timestamps = 1;
				break;
			case 'v':
                		parsed_args++;
				version(argv[0]);
				exit(1);
				break;
			case 'w':
                		parsed_args++;
				writeonly = 1;
				printf (" deamon ");
				break;
			case 'x':
                		parsed_args++;
				debug = atoi(optarg);
				printf ("debug level: %d",debug);
				break;
			case 'u':
			case '?':
                		parsed_args++;
				usage(argv[0]);
				exit(1);
				break;
			default:
				printf ("Error: getopt returned character code 0%o ??\n", c);
				printf ("Option not %c supported\n", c);
				exit(1);
				break;


			} //\switch
		
  /*              if (argc>parsed_args+3) {
			 printf ("\nError: getopt reurned %d arguments, only parsed_args %d were handed\n.",argc, parsed_args);
			usage(argv[0]);
			exit(1);
			}
*/
		
                if (argc<2) {
			 printf ("\nError: no arguments were given\n\n.");
			usage(argv[0]);
			exit(1);
			}
	
		if (loud) printf("optind %d",optind);	

		sdevname = argv[optind];
		remotefd = (int *) malloc (maxConnects * sizeof(int));
		if (loud) {
			printf("\n\nSettings:\n");	
			printf("Server to connect to: %s\n",machinename);
			printf("TCP-Port: %d\t",port);
			printf("stty-parameters: %s\t",sttyparms);
			printf("Time of Program start %d ",timestartms);
 			printf("\n\n"); //now the tool starts to operate: we want some nice newlines.
		}
		
		if (hexdump>0) {
			printf(" Received Data                          Transmitted Data\n");  
		}


		// struct group *getgrgid(gid_t gid);

		/*
		printf("sdevname=%s,port=%d,stty=%s\n",sdevname,port,sttyparms);
		*/

		openlog("remserial", LOG_PID, LOG_USER);

		if (writeonly)
			devfd = open(sdevname,O_WRONLY);
		else
			devfd = open(sdevname,O_RDWR);
		if ( devfd == -1 ) {
			syslog(LOG_ERR, "Open of %s failed: %m",sdevname);
			exit(1);
		}

		if (linkname)
			link_slave(devfd);

		if ( sttyparms ) {
			set_tty(devfd,sttyparms);
		}

		signal(SIGINT,sighandler);
		signal(SIGHUP,sighandler);
		signal(SIGTERM,sighandler);

		if ( machinename ) {
			/* We are the client,
			   Find the IP address for the remote machine */
			remotehost = gethostbyname(machinename);
			if ( !remotehost ) {
				syslog(LOG_ERR, "Couldn't determine address of %s",
					machinename );
				exit(1);
			}

			/* Copy it into the addr structure */
			addr.sin_family = AF_INET;
			memcpy(&(addr.sin_addr),remotehost->h_addr_list[0],
				sizeof(struct in_addr));
			addr.sin_port = htons(port);

			remotefd[curConnects++] = connect_to(&addr);
		}
		else {
			/* We are the server */

			/* Open the initial socket for communications */
			sockfd = socket(AF_INET, SOCK_STREAM, 6);
			if ( sockfd == -1 ) {
				syslog(LOG_ERR, "Can't open socket: %m");
				exit(1);
			}

			addr.sin_family = AF_INET;
			addr.sin_addr.s_addr = 0;
			addr.sin_port = htons(port);
		 
			/* Set up to listen on the given port */
			if( bind( sockfd, (struct sockaddr*)(&addr),
				sizeof(struct sockaddr_in)) < 0 ) {
				syslog(LOG_ERR, "Couldn't bind port %d, aborting: %m",port );
				exit(1);
			}
			if ( debug>1 )
				syslog(LOG_NOTICE,"Bound port");

			/* Tell the system we want to listen on this socket */
			result = listen(sockfd, 4);
			if ( result == -1 ) {
				syslog(LOG_ERR, "Socket listen failed: %m");
				exit(1);
			}

			if ( debug>1 )
				syslog(LOG_NOTICE,"Done listen");
		}

		if ( isdaemon ) {
			setsid();
			close(0);
			close(1);
			close(2);
		}

		/* Set up the files/sockets for the select() call */
		if ( sockfd != -1 ) {
			FD_SET(sockfd,&fdsread);
			if ( sockfd >= maxfd )
				maxfd = sockfd + 1;
		}

		for (i=0 ; i<curConnects ; i++) {
			FD_SET(remotefd[i],&fdsread);
			if ( remotefd[i] >= maxfd )
				maxfd = remotefd[i] + 1;
		}

		if (!writeonly) {
			FD_SET(devfd,&fdsread);
			if ( devfd >= maxfd )
				maxfd = devfd + 1;
		}

		while (1) {

			/* Wait for data from the listening socket, the device
			   or the remote connection */
			fdsreaduse = fdsread;
			if ( select(maxfd,&fdsreaduse,NULL,NULL,NULL) == -1 )
				break;

			/* Activity on the controlling socket, only on server */
			if ( !machinename && FD_ISSET(sockfd,&fdsreaduse) ) {
				int fd;

				/* Accept the remote systems attachment */
				remoteaddrlen = sizeof(struct sockaddr_in);
				fd = accept(sockfd,(struct sockaddr*)(&remoteaddr),
					&remoteaddrlen);

				if ( fd == -1 )
					syslog(LOG_ERR,"accept failed: %m");
				else if (curConnects < maxConnects) {
					unsigned long ip;

					remotefd[curConnects++] = fd;
					/* Tell select to watch this new socket */
					FD_SET(fd,&fdsread);
					if ( fd >= maxfd )
						maxfd = fd + 1;
					ip = ntohl(remoteaddr.sin_addr.s_addr);
					syslog(LOG_NOTICE, "Connection from %d.%d.%d.%d",
						(int)(ip>>24)&0xff,
						(int)(ip>>16)&0xff,
						(int)(ip>>8)&0xff,
						(int)(ip>>0)&0xff);
				}
				else {
					// Too many connections, just close it to reject
					close(fd);
				}
			}

			/* Data to read from the device */
			if ( FD_ISSET(devfd,&fdsreaduse) ) {

				devbytes = read(devfd,devbuf,DEVBUFSIZE);
			//if ( debug>1 && devbytes>0 )
			if (debug>1)
				syslog(LOG_INFO,"Device: %d bytes",devbytes);

//hexdump		
			if ( hexdump==1) {
			//hexdump device read to console
			//devbytes has number of valid bytes in devbuf, that we hexdump.
			dumpbuf(devbytes, devbuf, 1);  //left
			}

//hexdump		
			if ( hexdump==2) {
			//hexdump to syslog
			}
//\hexdump


//HOOK FOR PARSER
/*Parser will be a prototype of a parser that does only memcopy.
A function pointer of that prototype gets called in order to parse the traffic and to
"copy" data from the inbuffer to the outbuffer.
We provide pointer to inbuffer, outbuffer and direction (TX or RX)
The rest like states is done by the parser using static variables
*/


			if ( devbytes <= 0 ) {
				if ( debug>0 )
					syslog(LOG_INFO,"%s closed",sdevname);
				close(devfd);
				FD_CLR(devfd,&fdsread);
				while (1) {
					devfd = open(sdevname,O_RDWR);
					if ( devfd != -1 )
						break;
					syslog(LOG_ERR, "Open of %s failed: %m",
						sdevname);
					if ( errno != EIO )
						exit(1);
					sleep(1);
				}
				if ( debug>0 )
					syslog(LOG_INFO,"%s re-opened",sdevname);
				if ( sttyparms )
					set_tty(devfd,sttyparms);
				if (linkname)
					link_slave(devfd);
				FD_SET(devfd,&fdsread);
				if ( devfd >= maxfd )
					maxfd = devfd + 1;
			}
			else
				for (i=0 ; i<curConnects ; i++)
					write(remotefd[i],devbuf,devbytes);
		}

		/* Data to read from the remote system */
		for (i=0 ; i<curConnects ; i++)
			if (FD_ISSET(remotefd[i],&fdsreaduse) ) {

				devbytes = read(remotefd[i],devbuf,DEVBUFSIZE);

				//if ( debug>1 && devbytes>0 )
				if (debug>1)
					syslog(LOG_INFO,"Remote: %d bytes",devbytes);


//hexdump		
			if ( hexdump==1) {
			//hexdump network read to console
			//devbytes has number of valid bytes in devbuf, that we hexdump.
			dumpbuf(devbytes, devbuf, 0);  //right
			}

//hexdump		
			if ( hexdump==2) {
			//hexdump to syslog
			}
//\hexdump


				if ( devbytes == 0 ) {
					register int j;

					syslog(LOG_NOTICE,"Connection closed");
					close(remotefd[i]);
					FD_CLR(remotefd[i],&fdsread);
					curConnects--;
					for (j=i ; j<curConnects ; j++)
						remotefd[j] = remotefd[j+1];
					if ( machinename ) {
						/* Wait for the server again */
						remotefd[curConnects++] = connect_to(&addr);
						FD_SET(remotefd[curConnects-1],&fdsread);
						if ( remotefd[curConnects-1] >= maxfd )
							maxfd = remotefd[curConnects-1] + 1;
					}
				}
				else if ( devfd != -1 )
					/* Write the data to the device */
					write(devfd,devbuf,devbytes);
		}
	}
	close(sockfd);
	for (i=0 ; i<curConnects ; i++)
		close(remotefd[i]);
}

void sighandler(int sig)
{
	int i;

	if ( sockfd != -1 )
		close(sockfd);
	for (i=0 ; i<curConnects ; i++)
		close(remotefd[i]);
	if ( devfd != -1 )
		close(devfd);
	if (linkname)
		unlink(linkname);
	syslog(LOG_ERR,"Terminating on signal %d",sig);
	exit(0);
}

void link_slave(int fd)
{
	char *slavename;
	int status = grantpt(devfd);
	if (status != -1)
		status = unlockpt(devfd);
	if (status != -1) {
		slavename = ptsname(devfd);
		if (slavename) {
			// Safety first
			unlink(linkname);
			status = symlink(slavename, linkname);
		}
		else
			status = -1;
	}
	if (status == -1) {
		syslog(LOG_ERR, "Cannot create link for pseudo-tty: %m");
		exit(1);
	}
}

int connect_to(struct sockaddr_in *addr)
{
	int waitlogged = 0;
	int stat;
	extern int errno;
	int sockfd;

	if ( debug>0 ) {
		unsigned long ip = ntohl(addr->sin_addr.s_addr);
		syslog(LOG_NOTICE, "Trying to connect to %d.%d.%d.%d",
			(int)(ip>>24)&0xff,
			(int)(ip>>16)&0xff,
			(int)(ip>>8)&0xff,
			(int)(ip>>0)&0xff);
	}

	while (1) {
		/* Open the socket for communications */
		sockfd = socket(AF_INET, SOCK_STREAM, 6);
		if ( sockfd == -1 ) {
			syslog(LOG_ERR, "Can't open socket: %m");
			exit(1);
		}

		/* Try to connect to the remote server,
		   if it fails, keep trying */

		stat = connect(sockfd, (struct sockaddr*)addr,
			sizeof(struct sockaddr_in));
		if ( debug>1 )
			if (stat == -1)
				syslog(LOG_NOTICE, "Connect status %d, errno %d: %m\n", stat,errno);
			else
				syslog(LOG_NOTICE, "Connect status %d\n", stat);

		if ( stat == 0 )
			break;
		/* Write a message to syslog once */
		if ( ! waitlogged ) {
			syslog(LOG_NOTICE,
				"Waiting for server on %s port %d: %m",
				machinename,port );
			waitlogged = 1;
		}
		close(sockfd);
		sleep(10);
	}
	if ( waitlogged || debug>0 )
		syslog(LOG_NOTICE,
			"Connected to server %s port %d",
			machinename,port );
	return sockfd;
}

void usage(char *progname) {
	version(progname);
	printf("Usage:\n");
	synopsis(progname);
	printf("-r machinename		The remote machine name to connect to.  If not\n");
	printf("			specified, then this is the server side.\n");
	printf("-p netport		Specifiy IP port# (default 23000)\n");
	printf("-s \"stty params\"	If serial port, specify stty parameters, see man stty\n");
	printf("-m max-connections	Maximum number of simultaneous client connections to allow\n");
	printf("-d			Run as a daemon program\n");
	printf("-e			Prints usage examples and exits.\n");
	printf("-q			Quiet, no chatty messages that flood the console\n");
	printf("-x debuglevel		Set debug level, 0 is default, 1,2 give more info\n");
	printf("-l linkname		If the device name is a pseudo-tty, create a link to the slave\n");
	printf("-t			print time stamps in hexview.\n");
	printf("-? or -u       		Prints Usage (this) and exits.\n");
	printf("-v          		Prints Version and exits\n");
	printf("-w          		Only write to the device, no reading\n");
	printf("-h          		Prints a Hexdump of the communication (in/out) to the console\n");
	printf("-H          		Prints a Hexdump of the communication (in/out) to syslog\n");
	printf("device			I/O device, either serial port or pseudo-tty master\n");

//	examples(progname);	//show examples
}


void version(char *progname) {
	printf("%s version 1.0\n",progname);
	printf("By Ludwig Jaffe based on remserial 1.4 by Paul Davis\n",progname);
	printf("License: GPLv2\n",progname);

}


void synopsis(char *progname) {
	printf("\nSynopsis:\n");
	printf("%s [-r machinename] [-p netport] [-s \"stty params\"] [-m maxconnect] [-q] [-w] [-h] [-H] device\n\n",progname);
}

void examples(char *progname) {
	version(progname);
	synopsis(progname);
	printf("\nExamples:\n\n");
	printf("Server:\n");
	printf("The Computer with attached serial device for data aquisition will be considered a server. It runs:\n");
	printf("	%s -d -p 23000 -s \"9600 raw\" /dev/ttyS0 & \n\n",progname);
	
	printf("Client with serial port:\n");
	printf("The Computer which gets the data from the server via tcp/ip is a client.\nIt forwards data from/to the server to/from its attached serial device. It runs:\n");
	printf("	%s -d -r server-name -p 23000 -s \"1200 raw\" /dev/ttyUSB0  & \n",progname);
	printf("Note: speed and serial settings may differ.\nTo this client bridges the serial port of the server to its serial port.\n\n");
	printf("Client wich fakes a serial port:\n");
	printf("The Computer which gets the data from the server via tcp/ip is a client.\n");
	printf("It forwards data from/to the server to/from a faked serial device, a symlink to /dev/ptmx,\n");
	printf("which is a slave pseudo terminal.\nThe client who wants to fool its own software with a fake serial runs:\n");
	printf("	%s -d -r server-name -p 23000 -l /dev/remserial1 /dev/ptmx &\n",progname);
	printf("Note: The fake serial does not need speed and serial setting as it ignores them.\n");
	printf("This client bridges the serial port of the server to its faked serial port.\n\n");
	printf("Hint: one can use telnet to connect to remserial and talk to the remote serial port:\ntelnet server-name 23000\n"); 
	printf("Note: make sure that remserial gets executed with sufficient rights. In doubt, run as root\n\n"); 
}



void dumpbuf(int count, char * buffer, int left_right)
{
    int c=' ';                    /* Character read from the file */
    int printed=0;	//number of chars read from buffer
    char *readptr;	//points into reading position of buffer
    int leftrightdistance=0;	//distance between left and right collum
    
/*
    these are global as we have right  and left pane
    char * hex_offset;     // Position of the next character in Hex
 
    char * ascii_offset;      // Position of the next character in ASCII.
 
    char line[81];        // O/P line.      
*/ 
    readptr=buffer; // pointer points to start of buffer

    if (left_right)	leftrightdistance=RIGHTLEFTOFFSET; else leftrightdistance=0; 
    while (printed < count )
    {
        clear_line(line, sizeof line);
        hex_offset   = line+HEX_OFFSET+leftrightdistance;
        ascii_offset = line+ASCII_OFFSET+leftrightdistance;
 
 //read characters from buffer to fill line
        while ( ascii_offset < line+ASCII_OFFSET+NUM_CHARS+leftrightdistance
                && printed<count )
        {
	    //read c from buffer
	    c=*readptr;readptr++;printed++;	

            /* Build the hex part of
             * the line.      */
            hex_offset = hex(hex_offset, c);
 
            /* Build the Ascii part of
             * the line.      */
            ascii_offset = ascii(ascii_offset, c);
 
        }
	if (timestamps) print_timestamp();
        printf("%s\n", line);
    }
}
 
void clear_line(char *line, int size)
{
    int count;
 
    for  (count=0; count < size; line[count]=' ', count++);
}
 
char * ascii(char *position, int c)
{
    /* If the character is NOT printable
     * replace it with a '.'  */
    if (!isprint(c)) c='.';
 
    sprintf(position, "%c", c);    /* Put the character to the line
                                    * so it can be displayed later */
 
    /* Return the position of the next
     * ASCII character.   */
    return(++position);
}
 
char * hex(char *position, int c)
{
    int offset=3;
 
    sprintf(position, "%02X ", (unsigned char) c);
 
    *(position+offset)=' ';   /* Remove the '/0' created by 'sprint'  */
 
    return (position+offset);
}

void print_timestamp()
{
	struct timeval  tv;
	double timediff;
	gettimeofday(&tv, NULL);
	timediff = ((tv.tv_sec) * 1000 + (tv.tv_usec) / 1000) - timestartms ; // convert tv_sec & tv_usec to ms
	printf("%f :",timediff);
}


