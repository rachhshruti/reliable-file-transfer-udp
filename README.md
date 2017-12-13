# Reliable UDP

Implemented sliding window protocol for providing flow control, congestion control and adaptive retransmission of packets to make User Datagram Protocol (UDP) reliable.

# Requirements

- Linux operating system

# Running the code

The code can be compiled by typing the following command in the terminal: make

## To run the UDPServer:

  `./Server port advertised_window`

The server accepts the following command-line arguments:

port: port number to be used for communication

advertised_window: the number of bytes server is allowed to send before waiting for an acknowldgement

## To run the UDPClient:

  `./Client server_host_name port file_name advertised_window`

The client accepts the following command-line arguments:

port and advertised_window: same as that of the server

server_host_name: host name of the server

file_name: the name of the file requested by the client
