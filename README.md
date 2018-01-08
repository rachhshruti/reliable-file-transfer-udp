# Reliable UDP

Implemented sliding window protocol for providing flow control, congestion control and adaptive retransmission of packets to make User Datagram Protocol (UDP) reliable.

# Running the code

## Creating and configuring virtual machines
	
	vagrant up
   
   This will boot both the server and client machines

## SSH into virtual machines
	
	vagrant ssh reliableUDPServer
	vagrant ssh reliableUDPClient
   
## Compile the code
	
	make

## To run reliable UDP server:

  `./Server port advertised_window`

The server accepts the following command-line arguments:

port: port number to be used for communication

advertised_window: the number of bytes server is allowed to send before waiting for an acknowldgement

## To run reliable UDP client:

  `./Client server_host_name port file_name advertised_window`

The client accepts the following command-line arguments:

port and advertised_window: same as that of the server

server_host_name: host name of the server

file_name: the name of the file requested by the client

Once the code is run successfully, you will find the file that the client requested for in the ReliableUDPClient folder.

# Screenshots

<img src="https://github.com/rachhshruti/reliable-file-transfer-udp/blob/master/images/reliable-file-transfer-udp-output.png" width="1000" height="600" align="center"/>

# References

[Computer Networking-Top-Down-Approach](https://www.amazon.com/Computer-Networking-Top-Down-Approach-6th/dp/0132856204)
