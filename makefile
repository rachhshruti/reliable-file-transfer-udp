all:
	g++ -o Server UDPServer.cpp
	g++ -o Client UDPClient.cpp
clean:
	rm -f Server
	rm -f Client
