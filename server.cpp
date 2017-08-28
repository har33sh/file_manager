// #include<iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define PORT 9511
#define BUFFER_SIZE 256

//Global paramaters
int sockfd, newsockfd, portno;
socklen_t clilen;
char buffer[256];
char response_message[100];
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

//////////////////////////////////THINGS TO BE CHANGED
bool checkusername_avail(char *username){
    return true;
}

bool reg(char *username,char *password){
    return true;
}

bool login(char *username,char *password){
    return true;
}


void load_file_list(){
    //loads file load_file_list
    snprintf(response_message, sizeof(response_message), "FILE 1 FILE 2....");
}
////////////////////////////////////////////////////////




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
    snprintf(response_message, sizeof(response_message), "Received Successfully");
}

void closeConnection(){
    close(newsockfd);
    close(sockfd);
}

void force_send_response(){ //Clients waits for message, can be used to send dummy messges
    int _=write(newsockfd,response_message,strlen(response_message));
}


//process message and sent response
void onMessage(char *buffers){
    printf("Here is the message: %s\n",buffer);
    int choice=  atoi(strtok(buffer, ","));
    char *msg1= strtok(NULL, ",");      //Assuming there are only 3 arg passed...
    char *msg2=strtok(NULL, ",");
    snprintf(response_message, sizeof(response_message), "--No message--");
    switch (choice) {
        case 1: if(checkusername_avail(msg1))
                    snprintf(response_message, sizeof(response_message), "true");
                else
                    snprintf(response_message, sizeof(response_message), "false");
                break;
        case 2: if (reg(msg1,msg2))
                    snprintf(response_message, sizeof(response_message), "true");
                 else
                    snprintf(response_message, sizeof(response_message), "false");
                 break;
        case 3: if (login(msg1,msg2))
                    snprintf(response_message, sizeof(response_message), "true");
                 else
                    snprintf(response_message, sizeof(response_message), "false");
                 break;
        case 4: fileRecieve(msg1);break;
        case 5: load_file_list(); break;
        case 6:  printf("File Request %s \n",msg1 );break;
        default : printf("Admin, we have a issue..! %d %s %s\n",choice,msg1,msg2);

    }

    printf("%d %s %s\n",choice,msg1,msg2 );
}

//need to add case statement and add a way to close the connenction
void onRecieve(){
        int n;
        while(true){
            bzero(buffer,256);
            n = read(newsockfd,buffer,255);
            if (n < 0) error("ERROR reading from socket");
            onMessage(buffer);
            n = write(newsockfd,response_message,strlen(response_message));
            if (n < 0) error("ERROR writing to socket");
        }
}


int main(int argc, char *argv[]){
     establishConenction();
     onRecieve();
    //  char filename[]="a.txt";
    //  fileRecieve(filename);
     closeConnection();
     return 0;
}
