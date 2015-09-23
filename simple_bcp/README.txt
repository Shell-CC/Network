ECEN602 Programming Assignment 1
----------------------------------

Team 24
Member 1 # Xiaoxi Guo (UIN: 824000546), tjuguoxiaoxi@tamu.edu.
Member 2 #  (UIN: )
--------------------------------------------------------------

Description
-----------
1. Implemented client and server as requirements.
2. Implemented Bonus 1: works for both IPv4 and IPv6.
3. Implemented Bonus 2: ACK, NCK, ONLINE, OFFLINE.

Architecture:
-------------
bcp_msg.h:       includes struct of SBSP Messaage, and macro define of TYPE.
bcp_msg.cpp:     includes method of packing and unpacking SBSP Message struct.
bcp_process.cpp: includes methods of processing sending and receiving message.
bcp_client.cpp:  client
bcp_server.cpp:  server

Build on Linux/Unix:
--------------------
Usage: 'make' or 'make all'

Run for the server
------------------
Usage: './server server_ip server_port max_clients'
1. The server can at least handle 25 clients. More clients will need bigger buffer size.
2. server_ip can be both IPv4 or IPv6.
3. The server will not quit until force quit from the process.

Run for the client
------------------
Usage: './client username server_ip server_port'
1. Use 'q\n' to exit from the server at any time after joining.
2. Duplicate username will cause the new client unconnecting to the server.
3. Exceeding client limits will also cause the new client unconnecting to the server.
4. Server Quit will cause all' clients quiting.