# Reliable UDP

Two transport protocols used to transfer files include: TCP and UDP. UDP is an unreliable protocol whereas TCP provides reliability, flow control, and congestion control, but it has an explicit connection establishment phase, which some application may not find desirable. The goal of this project is to design and implement a file transfer application which has all the good features of TCP without the connection establishment phase.

# Features

## Header Format design

The UDP Server and Client uses the following header format:

<img src="https://github.com/rachhshruti/reliable-file-transfer-udp/blob/master/images/header.png" width="700" height="200" align="center"/>

* Sequence Number: This is the 32-bit sequence number of the segment that is to be sent.
* Acknowledgement Number: This is the 32-bit acknowledgement number for the correctly received segment.
* Ack Flag: This is the 8-bit Ack flag which is set to 1 when an acknowledgement is sent and set to 0 when data is sent.
* Data: This is 1450 byte data field which is used to store the file data. This field is not used in case an acknowledgement is being sent.

## Sliding window algorithm

The server receives file request from the client. It extracts the file name from the request and gets the content of the file in a buffer. It then segments the file into chunks of size 1450 bytes as supported by the header.

There are two window sizes to be considered:
1. Congestion window: The number of bytes the server can send without congesting the routers in the middle of network path.
2. Advertised window: The number of bytes the client can receive without requiring to send an acknowledgement. It is decided offline between server and client and accepted as the command-line argument in this project.

The server starts sending file segments till the minimum of the congestion window size and advertised window size. After sending all the segments in a window, it waits to receive an acknowledgement. When an acknowledgement is received for a sequence number that is less than the next sequence number that is to be sent, then it means some previous segment was not correctly received and the server accepts the 3 duplicate acknowledgements from the client. It then sets the next sequence number based on the acknowledgement received. After receiving the cumulative acknowledgement, it continues sending the segments in the next window. When duplicate acknowledgements are received, the server will start sending segments from the packet that was not correctly received.

## Adaptive Retransmission

Implemented Jacobson/Karel's algorithm for estimating the round trip time (RTT) for the packets to be recieved by the client and acknowledgment received by the server. If the server does not get acknowledgement within RTT, it will retransmit those packets again.

Pseudo code:
~~~~
Estimated_RTT = (1-α) Estimated RTT + (α) Sample_RTT
In the original TCP Specification, α=.0125

Jacobson/Karels included a variation component to the calculation for the Estimated_RTT
Estimated_RTT = Estimated_RTT + δ (Sample_RTT-Estimated_RTT) 
Deviation = Deviation + δ (|Sample_RTT- Estimated_RTT|- Deviation) 
Timeout = μ * Estimated_RTT + φ * Deviation
Typically φ=4, μ = 1, δ is between 0 and 1
~~~~

## Congestion control
Server starts by sending 1 segment. Then, follows 2 phases of congestion control:

1. __Slow Start:__ Segment size is increased exponentially until it is correctly acknowledged by the receiver within the timeout and the congestion window size is less than the ssthresh which is set to 64000 bytes.

2. __Congestion Avoidance:__ When the congestion window size becomes greater than or equal to ssthresh, the congestion
window size is increased linearly by 1 segment size.

__Timeout:__ It is calculated using the Jacobson/Karel's algorithm given above. When a timeout occurs, the ssthresh is set to half the value of the congestion window and the congestion window is reset to 1 segment size.

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
