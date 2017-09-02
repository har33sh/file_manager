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
#define FILE_SERVER_PORT 9559
#define PORT 4559
#define BUFFER_SIZE 256
char file_dir[]="/home/ghost/Downloads/Data";
char file_list[]="/home/ghost/file_list.txt";

// int PORT,FILE_SERVER_PORT;

//Global paramaters
int sockfd, newsockfd, portno;
struct sockaddr_in serv_addr, cli_addr;
socklen_t clilen;
char file_buffer[BUFFER_SIZE],send_message[BUFFER_SIZE],response_message[BUFFER_SIZE];
char buffer[BUFFER_SIZE],auth_user[100];
using namespace std;

int sockfd_file;
struct sockaddr_in file_serv_addr;
struct hostent *fileserver;

//\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\
////////////////////////////////   Network Layer  \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\
//\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\

void error(const char *msg){
    perror(msg);
    exit(1);
}
//////////////////////////////File Sever Connection ////////////////////////////////////////////////
char proxy_forward_msg[BUFFER_SIZE],rev_proxy_forward_msg[BUFFER_SIZE];
int forwaded_bytes,fs_recv_bytes;

void receiveMessage();
void sendMessage();

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
    forwaded_bytes = write(sockfd_file,proxy_forward_msg,strlen(proxy_forward_msg));
    printf("\t----->%d %s\n",forwaded_bytes,"proxy_forward_msg");
    if (forwaded_bytes < 0)
        error("ERROR writing to socket");
}

void recvFileServerMessage(){
    bzero(rev_proxy_forward_msg,BUFFER_SIZE);
    fs_recv_bytes= read(sockfd_file,rev_proxy_forward_msg,BUFFER_SIZE);
    printf("\t<-----%d %s\n",fs_recv_bytes,"rev_proxy_forward_msg");
    if (fs_recv_bytes < 0)
        error("ERROR reading from socket");
}



void closeFileServerConnection(){
    snprintf(proxy_forward_msg, sizeof(proxy_forward_msg), "%s,%s", "0","Byee..");
    int n = write(sockfd_file,proxy_forward_msg,strlen(proxy_forward_msg));
    close(sockfd_file);
}


//forwards download request to file server
void start_proxy_server(){
    pid_t pid;
    pid = fork();
    if (pid==0){
        while (true){
            receiveMessage();
            forwardMessage();
        }
    }
    else{
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
        printf("Client conneced \n" );
        pid = fork();
        printf("Spawning new process\n" );
        if (newsockfd < 0)
            error("ERROR on accept");
        if(pid!=0){
            close(newsockfd);
            continue;
        }
        else{
            printf("Spawning new process for client\n" );
            connectToFileServer();
            break;
        }
    }

}


void closeConnection(){
    closeFileServerConnection();
    close(newsockfd);
    close(sockfd);
    printf("Closed all connections.." );
}



void sendMessage(){
        int bytes_written = write(newsockfd,rev_proxy_forward_msg,strlen(rev_proxy_forward_msg));
        printf("<-----%d %s\n",bytes_written,"send_message");
        if (bytes_written < 0)
            error("ERROR writing to socket");
}

void receiveMessage(){
    bzero(proxy_forward_msg,BUFFER_SIZE);
    int bytes_read = read(newsockfd,proxy_forward_msg,BUFFER_SIZE);
    printf("----->%d %s\n",bytes_read,"response_message");
    if (bytes_read < 0)
        error("ERROR reading from socket");
}


int main(int argc, char *argv[]){
    scanf("%d",&PORT );
    scanf("%d",&FILE_SERVER_PORT );
    // connectToFileServer();
     establishConenction();

     start_proxy_server();

     closeConnection();
    // closeFileServerConnection();
     return 0;
}
