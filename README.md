# remserial2
fork and extension of remserial, which is a serial interface to tcp port redirector and vice versa. Additions are hex view (time stamping will come soon) of dataflow and better help functions and documentation

If you see interrupted lines with only some chars printed in hex this means that the 
tcp transport was quick enough to empty the buffers so the program received and sent the data
in one cycle.
If there is much data printed in one field (tx-side or rx-side) this means that the data
is stuck somewhere and was not fetched by the other side in time.

It is best to see only a short block for tx side and for rx side and much ping-pong
which shows healthy communication.

get info on remserial2 using remserial2 -e

./remserial2 version 1.0
By Ludwig Jaffe based on remserial 1.4 by Paul Davis
License: GPLv2

Synopsis:
./remserial2 [-r machinename] [-p netport] [-s "stty params"] [-m maxconnect] [-q] [-w] [-h] [-H] device


Examples:

Server:
The Computer with attached serial device for data aquisition will be considered a server. It runs:
	./remserial2 -d -p 23000 -s "9600 raw" /dev/ttyS0 & 

Client with serial port:
The Computer which gets the data from the server via tcp/ip is a client.
It forwards data from/to the server to/from its attached serial device. It runs:
	./remserial2 -d -r server-name -p 23000 -s "1200 raw" /dev/ttyUSB0  & 
Note: speed and serial settings may differ.
To this client bridges the serial port of the server to its serial port.

Client wich fakes a serial port:
The Computer which gets the data from the server via tcp/ip is a client.
It forwards data from/to the server to/from a faked serial device, a symlink to /dev/ptmx,
which is a slave pseudo terminal.
The client who wants to fool its own software with a fake serial runs:
	./remserial2 -d -r server-name -p 23000 -l /dev/remserial1 /dev/ptmx &
Note: The fake serial does not need speed and serial setting as it ignores them.
This client bridges the serial port of the server to its faked serial port.

Hint: one can use telnet to connect to remserial and talk to the remote serial port:
telnet server-name 23000
Note: make sure that remserial gets executed with sufficient rights. In doubt, run as root


