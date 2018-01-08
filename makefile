all:
	g++ -o ReliableUDPServer/Server ReliableUDPServer/UDPServer.cpp
	g++ -o ReliableUDPClient/Client ReliableUDPClient/UDPClient.cpp
clean:
	rm -f Server
	rm -f Client
