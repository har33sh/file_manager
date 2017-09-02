#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include<dirent.h>
#include <string.h>
#include <netdb.h>
#include <string>
#include <vector>
#include <wordexp.h>//for word expansions
#include <utility>
#include <errno.h>
#include <arpa/inet.h>
#include <iterator>
#include <cctype>
#include <algorithm>
#include <sstream>
#include <fstream>
#include <ctime>
#include <fcntl.h>
#include <stdarg.h>


#include "db.h"

using namespace std;

//config
#define FILE_SERVER "10.129.23.200"
#define FILE_SERVER_PORT 9333
#define PORT 4333
#define BUFFER_SIZE 256
char file_dir[]="/home/ghost/Downloads/Data";
char file_list[]="/home/ghost/file_list.txt";


//Global paramaters
int client_reconnnect=10,server_reconnect=10;
int sockfd, newsockfd, portno;
struct sockaddr_in serv_addr, cli_addr;
socklen_t clilen;
char file_buffer[BUFFER_SIZE],send_message[BUFFER_SIZE],response_message[BUFFER_SIZE];
char buffer[BUFFER_SIZE],auth_user[100];
using namespace std;

int sockfd_file;
struct sockaddr_in file_serv_addr;
struct hostent *fileserver;

//declarations
void closeFileServerConnection();
void receiveMessage();
void sendMessage();
void home_page();

//\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\
////////////////////////////////   Network Layer  \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\
//\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\

void error(const char *msg){
    perror(msg);
    exit(1);
}
//////////////////////////////File Sever Connection ////////////////////////////////////////////////

char msg_from_client[BUFFER_SIZE],msg_to_fs[BUFFER_SIZE],msg_to_client[BUFFER_SIZE],msg_from_fs[BUFFER_SIZE];
int forwaded_bytes,fs_recv_bytes;

//Establish connection to server
void connectToFileServer(){
    sockfd_file = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd_file < 0)
    error("ERROR opening socket");
    fileserver = gethostbyname(FILE_SERVER);
    if (fileserver == NULL) {
        fprintf(stderr,"ERROR,File server not found\n");
        exit(0);
    }
    bzero((char *) &file_serv_addr, sizeof(file_serv_addr));
    file_serv_addr.sin_family = AF_INET;
    bcopy((char *)fileserver->h_addr,
    (char *)&file_serv_addr.sin_addr.s_addr,
    fileserver->h_length);
    file_serv_addr.sin_port = htons(FILE_SERVER_PORT);
    if (connect(sockfd_file,(struct sockaddr *) &file_serv_addr,sizeof(file_serv_addr)) < 0)
    error("ERROR connecting");
}


void forwardMessage(){
    forwaded_bytes = write(sockfd_file,msg_to_fs,strlen(msg_to_fs));
    printf("\t----->%d %s\n",forwaded_bytes,"msg_to_fs");
    if (forwaded_bytes < 0)
        error("ERROR writing to socket");
}

void recvFileServerMessage(){
    bzero(msg_from_fs,BUFFER_SIZE);
    fs_recv_bytes= read(sockfd_file,msg_from_fs,BUFFER_SIZE);
    if (fs_recv_bytes < 0)
        error("ERROR reading from socket");
    bzero(msg_to_client,BUFFER_SIZE);
    snprintf(msg_to_client,fs_recv_bytes+1,"%s",msg_from_fs);
    printf("\t<-----%d %s\n",fs_recv_bytes,"msg_from_fs");
    if (fs_recv_bytes==0){ // handeling closed connections
      server_reconnect-=1;
      if(server_reconnect==0){
        printf("Server has gone away \n" );
        closeFileServerConnection();
        home_page();
      }
    }
}



void closeFileServerConnection(){
    close(sockfd_file);
}


//forwards download request to file server
void start_proxy_server(){
    pid_t pid;
    pid = fork();
    if (pid==0){ //child process
        while (true){
            receiveMessage();
            forwardMessage();
        }
    }
    else{ //parent process
        while(true){
            recvFileServerMessage();
            sendMessage();
        }
    }

}


//////////////////////////////Connection to Clients////////////////////////////////////////////////

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
    pid_t pid;
    label: printf("Server started... Waiting for connection...\n");
    for(;;){
        newsockfd = accept(sockfd,
            (struct sockaddr *) &cli_addr,
            &clilen);
        printf("Client conneced :: Main process forked\n" );
        pid = fork();
        if (newsockfd < 0)
            error("ERROR on accept");
        if(pid!=0){
            close(newsockfd);
            continue;
        }
        else{
            printf("Moving the process control for client :: Child \n" );
            connectToFileServer();
            break;
        }
    }

}


void closeConnection(){ //closes connection with client and proxy server
    close(newsockfd);
    close(sockfd);
    printf("Closed all connections.." );
}



void sendMessage(){
        int bytes_written = write(newsockfd,msg_to_client,strlen(msg_to_client));
        printf("<-----%d %s\n",bytes_written,"msg_to_client" );
        if (bytes_written < 0)
            error("ERROR writing to socket");
}

void receiveMessage(){
    bzero(msg_from_client   ,BUFFER_SIZE);
    int bytes_read = read(newsockfd,msg_from_client,BUFFER_SIZE);
    printf("----->%d %s\n",bytes_read,"msg_from_client");
    if (bytes_read < 0)
        error("ERROR reading from socket");
    bzero(msg_to_fs   ,BUFFER_SIZE);
    snprintf(msg_to_fs,bytes_read+1,"%s",msg_from_client);
    if(bytes_read==0){
      client_reconnnect-=1;
      if(client_reconnnect==0){
        printf("Client has gone away..!!\n" );
        closeConnection();
      }
    }
}


//\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\
////////////////////////////////   Authentication  \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\
//\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\


void home_page(){
  printf("***** User Credentials ***** \n" );
    start_homepage: receiveMessage();
    char buffer[BUFFER_SIZE],response_message[BUFFER_SIZE];
    snprintf(buffer,sizeof(buffer),"%s",msg_from_client);
    printf("Message Recv : %s\n",buffer);
    int choice=  atoi(strtok(buffer, ","));
    char *username= strtok(NULL, ",");
    char *password=strtok(NULL, ",");

    switch (choice) {
        case 1: if(usernameAvailable(username))
                    snprintf(response_message, sizeof(response_message), "true");
                else
                    snprintf(response_message, sizeof(response_message), "false");
                break;
        case 2: if (!createUser(username,password))
                    snprintf(response_message, sizeof(response_message), "true");
                 else
                    snprintf(response_message, sizeof(response_message), "false");
                 break;
        case 3: if (authenticateUser(username,password)){
                    snprintf(msg_to_client, sizeof(msg_to_client), "true"); //Sharing the buffer,
                    sendMessage();
                    return ;
                    }
                 else
                    snprintf(response_message, sizeof(response_message), "false");
                 break;

        case 0: closeConnection();exit(0);
        default: printf("Root, We have a problem...!\n");
                 printf("%d %s %s\n",choice,username,password );
                 closeConnection();
    }
    snprintf(msg_to_client, sizeof(msg_to_client), "%s",response_message);
    sendMessage();
    goto start_homepage;
}

//////////////////////////////////// Reserved for Main fun /////////////////////////////////////////////////

int main(int argc, char *argv[]){
     establishConenction();
     home_page();
     printf("!!!!!!!!!!!!!!! The place of smokes !!!!!!!!!!!!!!!" );
     start_proxy_server();
     closeConnection();
     return 0;
}
