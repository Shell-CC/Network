all: server client

server: bcp_server.o bcp_process.o bcp_msg.o
	g++ -o server bcp_server.o bcp_process.o bcp_msg.o
client: bcp_client.o bcp_process.o bcp_msg.o
	g++ -o client bcp_client.o bcp_process.o bcp_msg.o
bcp_process.o: bcp_process.cpp bcp_process.h
	g++ -Wall -c bcp_process.cpp
bcp_msg.o: bcp_msg.cpp bcp_msg.h
	g++ -Wall -c bcp_msg.cpp

clean:
	rm -f server client *.o