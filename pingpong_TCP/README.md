- TCPclient.c: Client code running on a desktop PC connecting to the CC3200, allowing sending out commands and receiving the response (hostname masked)
- TCPserv.c: Server code running on a desktop PC allowing the CC3200 to connect (not useful with the current implementation where the CC3200 runs a TCP server)

Compiler: g++ (cygwin)
