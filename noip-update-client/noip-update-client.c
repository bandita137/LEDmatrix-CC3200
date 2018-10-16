// No-IP update client
// Based on the NoIP update API:
// - https://www.noip.com/integrate/request
// - https://www.noip.com/integrate/response
// and https://stackoverflow.com/questions/22077802/simple-c-example-of-doing-an-http-post-and-consuming-the-response

#include <stdio.h> /* printf, sprintf */
#include <stdlib.h> /* exit */
#include <unistd.h> /* read, write, close */
#include <string.h> /* memcpy, memset */
#include <sys/socket.h> /* socket, connect */
#include <netinet/in.h> /* struct sockaddr_in, struct sockaddr */
#include <netdb.h> /* struct hostent, gethostbyname */

void error(const char *msg) { perror(msg); exit(0); }

int main(int argc,char *argv[])
{
    /* first what are we going to send and where are we going to send it? */
    int portno = 80;
    const char * host = "dynupdate.no-ip.com";
    const char * message = "GET /nic/update?hostname=HOSTNAME MASKED HTTP/1.0\r\nHost: dynupdate.no-ip.com\r\nAuthorization: Basic AUTH BASE MASKED\r\nUser-Agent:CH-noip-update-client Win10/0.01a E-MAIL MASKED\r\n\r\n";

    struct hostent *server;
    struct sockaddr_in serv_addr;
    int sockfd, bytes, sent, received, total;
    char response[1024];

    /* create the socket */
    printf("Opening socket...\r\n");
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) error("ERROR opening socket");

    /* lookup the ip address */
    printf("Looking up the IP address of %s ... ", host);
    server = gethostbyname(host);
    if (server == NULL) error("ERROR, no such host");
    printf("%d.%d.%d.%d\r\n", (unsigned char) (server -> h_addr[0]), (unsigned char) (server -> h_addr[1]), (unsigned char) (server -> h_addr[2]), (unsigned char) (server -> h_addr[3]));

    /* fill in the structure */
    memset(&serv_addr,0,sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(portno);
    memcpy(&serv_addr.sin_addr.s_addr,server->h_addr,server->h_length);

    /* connect the socket */
    printf("Connecting socket...\r\n");
    if (connect(sockfd,(struct sockaddr *)&serv_addr,sizeof(serv_addr)) < 0)
        error("ERROR connecting");

    /* send the request */
    printf("Sending request:\r\n");
    printf("%s", message);
    total = strlen(message);
    sent = 0;
    do {
        bytes = write(sockfd,message+sent,total-sent);
        if (bytes < 0)
            error("ERROR writing message to socket");
        if (bytes == 0)
            break;
        sent+=bytes;
    } while (sent < total);

    /* receive the response */
    printf("Listening for response...\r\n");
    memset(response,0,sizeof(response));
    total = sizeof(response)-1;
    received = 0;
    do {
        bytes = read(sockfd,response+received,total-received);
        if (bytes < 0)
            error("ERROR reading response from socket");
        if (bytes == 0)
            break;
        received+=bytes;
    } while (received < total);

    if (received == total)
        error("ERROR storing complete response from socket");

    /* close the socket */
    close(sockfd);

    /* process response */
    printf("Response:\n%s\n",response);

    return 0;
}
