#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#define HOST "10.129.23.200"
#define PORT 9512

int sockfd, portno, n;
struct sockaddr_in serv_addr;
struct hostent *server;
char buffer[256];

void error(const char *msg){
    perror(msg);
    exit(0);
}

void establishConenction(){
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
    error("ERROR opening socket");
    server = gethostbyname(HOST);
    if (server == NULL) {
        fprintf(stderr,"ERROR,Host not found\n");
        exit(0);
    }
    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr,
    (char *)&serv_addr.sin_addr.s_addr,
    server->h_length);
    serv_addr.sin_port = htons(PORT);
    if (connect(sockfd,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0)
    error("ERROR connecting");
}

int sendMessage(char *msg){
        n = write(sockfd,msg,strlen(buffer));
        if (n < 0)
        error("ERROR writing to socket");
        bzero(buffer,256);
        n = read(sockfd,buffer,255);
        if (n < 0)
        error("ERROR reading from socket");
        return atoi(buffer);
}

void closeConnection(){
    close(sockfd);
}

int main(int argc, char *argv[]){
    establishConenction();
    while(true){
        printf("Please enter the message: ");
        bzero(buffer,256);
        fgets(buffer,255,stdin);
        int response=sendMessage(buffer);
        printf("Response code %d \n",response);
    }
    closeConnection();
    return 0;
}
