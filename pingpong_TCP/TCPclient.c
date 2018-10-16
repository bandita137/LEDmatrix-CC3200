// TCP server code to communicate with the CC3200-LAUNCHXL

// Sockets code based on https://www.programminglogic.com/example-of-client-server-program-in-c-using-sockets-and-tcp/
// and http://www.cs.rpi.edu/~moorthy/Courses/os98/Pgms/socket.html

// More reference: http://beej.us/guide/bgnet/


#include <stdio.h>      // printf, sprintf
#include <stdlib.h>     // exit
#include <unistd.h>     // read, write, close
#include <string.h>     // memcpy, memset
#include <sys/socket.h> // socket, connect
#include <netinet/in.h> // struct sockaddr_in, struct sockaddr
#include <netdb.h>      // struct hostent, gethostbyname
#include <arpa/inet.h>
#include <unistd.h>     // close function

void error(const char *msg) { perror(msg); exit(0); }

int main() {
  const char* ServerHostName = "HOSTNAME MASKED";
  const int TCPPort = 7891;
  
  char buf_in[1024], buf_out[1024];

  struct hostent *Server;
  struct sockaddr_in ServerAddr;
  int CommSocket;
  int bytes, sent, received, total;
  char response[1024];


  // Create socket
  CommSocket = socket(AF_INET, SOCK_STREAM, 0);
  if (CommSocket < 0)
    error("ERROR opening socket");

  // IP address lookup
  Server = gethostbyname(ServerHostName);
  if (Server == NULL)
    error("ERROR, no such host");

  // Fill in the address structure
  memset(&ServerAddr, 0, sizeof(ServerAddr));
  ServerAddr.sin_family = AF_INET;
  ServerAddr.sin_port = htons(TCPPort);
  memcpy(&ServerAddr.sin_addr.s_addr, Server->h_addr, Server->h_length);

  // Connect the socket
  printf("Connecting to %s, port %d (%d.%d.%d.%d:%d) ...\n", ServerHostName, TCPPort, (unsigned char) (Server -> h_addr[0]), (unsigned char) (Server -> h_addr[1]), (unsigned char) (Server -> h_addr[2]), (unsigned char) (Server -> h_addr[3]), TCPPort);
  if (connect(CommSocket, (struct sockaddr *) &ServerAddr, sizeof(ServerAddr)) < 0)
    error("ERROR connecting");
  
  printf("... connected.\n");

  while (1) {
    gets(buf_out);

    if (!strcmp(buf_out, "DISC"))
      break;
    
    send(CommSocket, buf_out, 1024, 0);
      
    if (!strcmp(buf_out, "POLL")) {
      recv(CommSocket, buf_in, 1024, 0);
        
      printf("%s", buf_in);
    }
  }
    
  close(CommSocket);

  return 0;
}
