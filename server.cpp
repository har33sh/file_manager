// #include<iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define PORT 9512
#define BUFFER_SIZE 256

//Global paramaters
int sockfd, newsockfd, portno;
socklen_t clilen;
char buffer[256];
struct sockaddr_in serv_addr, cli_addr;

using namespace std;

void error(const char *msg){
    perror(msg);
    exit(1);
}

//socket, bind, listen
void establishConenction(){
    printf("Starting server ....\n");
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
    error("ERROR opening socket");
    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(PORT);
    if (bind(sockfd, (struct sockaddr *) &serv_addr,
    sizeof(serv_addr)) < 0)
    error("ERROR on binding");
    listen(sockfd,5);
    clilen = sizeof(cli_addr);
    printf("Server started... Waiting for connection...\n");
    newsockfd = accept(sockfd,
        (struct sockaddr *) &cli_addr,
        &clilen);
    if (newsockfd < 0)
        error("ERROR on accept");
}

//process message and sent response
int onMessage(char *buffers){
    printf("Here is the message: %s\n",buffer);
    return 0;
}

//need to add case statement and add a way to close the connenction
void onRecieve(){
        int n;
        while(true){
            bzero(buffer,256);
            n = read(newsockfd,buffer,255);
            if (n < 0) error("ERROR reading from socket");
            char response[3];
            sprintf(response,"%d",onMessage(buffer));
            n = write(newsockfd,response,strlen(response));
            if (n < 0) error("ERROR writing to socket");
        }
}

void fileRecieve(char *filename){
    FILE *fp;
    int n;
    fp = fopen(filename, "w");
    printf("Downloading... %s",filename);
    bzero(buffer,BUFFER_SIZE);
    while (n = read(newsockfd,buffer,BUFFER_SIZE) > 0 ){
        n = fwrite(buffer, sizeof(char), sizeof(buffer), fp);
        }
    fclose(fp);

}

void closeConnection(){
    close(newsockfd);
    close(sockfd);
}


int main(int argc, char *argv[]){
     establishConenction();
     char filename[]="a.txt";
     fileRecieve(filename);
     closeConnection();
     return 0;
}
