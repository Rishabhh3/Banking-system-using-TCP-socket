# Banking-system-using-TCP-socket

This is a Client Admin Banking system using TCP sockets

To run server : gcc server.c common/logger.c -o server
and then ./server 8080

To run client : gcc client.c -o client
and then ./client 127.0.0.1 8080