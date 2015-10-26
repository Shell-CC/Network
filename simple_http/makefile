all: proxy client

proxy: proxy.o process.o http_msg.o
	g++ -Wall -o proxy proxy.o process.o http_msg.o
client: client.o process.o http_msg.o
	g++ -o client client.o process.o http_msg.o
process.o: process.cpp process.h
	g++ -Wall -c process.cpp
http_msg.o: http_msg.cpp http_msg.h
	g++ -Wall -c http_msg.cpp

clean:
	rm -f proxy client *.o