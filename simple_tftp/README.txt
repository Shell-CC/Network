ECEN602 Programming Assignment 2
----------------------------------

Team 24
Member 1 # Xiaoxi Guo (UIN: 824000546), tjuguoxiaoxi@tamu.edu.
Member 2 # Aditya Kamath (UIN:824000196), adityakamath@tamu.edu
--------------------------------------------------------------

Description
-----------
1. Implemented server as requirements.
2. Implemented works both for IPv4 and IPv6.
3. Mode "netascii" and "octet" are supported, "mail" mode will be handled as error.
4. "WRQ" are not implemented yet.

Architecture:
-------------
tftp_server.c:  all implementations.

Build on Linux/Unix:
--------------------
Usage: 'make' or 'make all'

Run for the server
------------------
Usage: './server server_ip server_port'
1. server_ip can be both IPv4 or IPv6.
2. server_port can be any port (besides 69) that is able to be bind.
3. The server will not quit until you force quit from the process.
