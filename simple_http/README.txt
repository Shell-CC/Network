ECEN602 Programming Assignment 1
----------------------------------

Team 24
Member 1 # Xiaoxi Guo (UIN: 824000546), tjuguoxiaoxi@tamu.edu.
Member 2 # Aditya Kamath (UIN:824000196), adityakamath@tamu.edu
--------------------------------------------------------------

Description
-----------
1. Implemented client and proxy as requirements.
2. Implemented Bonus: by conditional get.
3. Expire implementation: expire header cant only be detected with format "Expire...\r\n", others like "expire" will be ignored, if no expire header, it will be treated as never expired.

Architecture:
-------------
http_msg.cpp:     includes method of packing and unpacking http message and url.
http_process.cpp: includes methods of processing sending and receiving message.
client.cpp:  client
proxy.cpp:  proxy with MAX cache size 10.

Build on Linux/Unix:
--------------------
Usage: 'make' or 'make all'

Run for the proxy
------------------
Usage: './proxy proxy_ip proxy_port'
1. The proxy use MAX_CACHE_SIZE = 10. This can be changed in MACRO define.
2. The proxy can at least handle 20 clients. This cant be changed in MACRO define.
3. proxy_ip can be both IPv4 or IPv6.
4. The server will not quit until force quit from the process.
5. After force quit, the cache file won't be removed. 

Run for the client
------------------
Usage: './client server_ip server_port url'
1. url must be in the format 'http(https)://<domain>/<page>'. <page> can be empty.
2. The received file will be saved to "client_recv".