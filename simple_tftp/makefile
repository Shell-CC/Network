all: server

server: tftp_server.c
	gcc -std=c99 -pthread -o server tftp_server.c

clean:
	rm -f server *.o