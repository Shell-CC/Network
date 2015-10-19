all: server

server: tftp_server.c
	gcc -pthread -o server tftp_server.c

clean:
	rm -f server *.o