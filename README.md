# Socket-Programming
A simple client-server application implemented using socket programming.</br>

How to run:</br>
- Open terminal in Server folder.
- Compile -> gcc server.c -o server -lpthread
- If port is used -> fuser -n tcp -k [port-number]
- Run -> ./server [port-number]. 
- Open terminal in Client folder.
- Compile -> gcc client.c -o client
- Run -> ./client [ip-address] [port-number
