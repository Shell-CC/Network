all: server client

server: bcp_server.o bcp_process.o
	g++ -o server bcp_server.o bcp_process.o
client: bcp_client.o bcp_process.o
	g++ -o client bcp_client.o bcp_process.o
bcp_process.o: bcp_process.cpp bcp_process.h
	g++ -Wall -c bcp_process.cpp

clean:
	rm -f server client *.o