// TCP server code to communicate with the CC3200-LAUNCHXL

// Sockets code based on https://www.programminglogic.com/example-of-client-server-program-in-c-using-sockets-and-tcp/
// and http://www.cs.rpi.edu/~moorthy/Courses/os98/Pgms/socket.html

// More reference: http://beej.us/guide/bgnet/


#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>     // close function

int main() {
  const int TCP_port = 7891;
  
  int welcomeSocket, commSocket;
  char buf_in[1024], buf_out[1024];
  struct sockaddr_in serverAddr;
  struct sockaddr_storage serverStorage;
  socklen_t addr_size;
  

  /*---- Create the socket. The three arguments are: ----*/
  /* 1) Internet domain 2) Stream socket 3) Default protocol (TCP in this case) */
  welcomeSocket = socket(PF_INET, SOCK_STREAM, 0);
  
  
  /*---- Configure settings of the server address struct ----*/
  /* Address family = Internet */
  serverAddr.sin_family = AF_INET;
  /* Set port number, using htons function to use proper byte order */
  serverAddr.sin_port = htons(TCP_port);
  /* Set IP address to localhost */
  serverAddr.sin_addr.s_addr = INADDR_ANY;
  /* Set all bits of the padding field to 0 */
  memset(serverAddr.sin_zero, '\0', sizeof serverAddr.sin_zero);  

  /*---- Bind the address struct to the socket ----*/
  bind(welcomeSocket, (struct sockaddr *) &serverAddr, sizeof(serverAddr));
  
  /*---- Listen on the socket, with 5 max connection requests queued ----*/
  if(listen(welcomeSocket,5)==0)
    printf("Listening on TCP port %d\n", TCP_port);
  else
    printf("Error\n");

  /*---- Accept call creates a new socket for the incoming connection ----*/
  addr_size = sizeof serverStorage;
  commSocket = accept(welcomeSocket, (struct sockaddr *) &serverStorage, &addr_size);
  
  if (serverStorage.ss_family == AF_INET) {
    uint32_t clientAddr = ntohl(((struct sockaddr_in *) &serverStorage) -> sin_addr.s_addr);
    uint32_t clientPort = ntohs(((struct sockaddr_in *) &serverStorage) -> sin_port);
    
    printf("Client connected: %d.%d.%d.%d:%d\n", (clientAddr >> 24) & 255, (clientAddr >> 16) & 255, (clientAddr >> 8) & 255, clientAddr & 255, clientPort);
    
    while (1) {
      gets(buf_out);
      
      send(commSocket, buf_out, 1024, 0);
      
      if (!strcmp(buf_out, "POLL")) {
        recv(commSocket, buf_in, 1024, 0);
        
        printf("%s", buf_in);
      }
        
      
      if (!strcmp(buf_out, "DISCONNECT"))
        break;
    }
    
    close(commSocket);
  }

  close(welcomeSocket);

  return 0;
}
