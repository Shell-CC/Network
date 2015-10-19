all: server

server: tftp_server.o process.o tftp_msg.o
	g++ -Wall -o server tftp_server.o process.o tftp_msg.o
process.o: process.cpp process.h
	g++ -Wall -c process.cpp
tftp_msg.o: tftp_msg.cpp tftp_msg.h
	g++ -Wall -c tftp_msg.cpp

clean:
	rm -f server *.o