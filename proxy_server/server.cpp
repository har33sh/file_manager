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
#include <unistd.h>
#include <sys/signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <sys/resource.h>
#include<signal.h>
#include <iostream>
#include "db.h"
#include <sys/prctl.h>
#include <pthread.h>

using namespace std;

//config

int FILE_SERVER_PORT,PORT;
char FILE_SERVER[50];
#define BUFFER_SIZE 256
char file_dir[]="/home/ghost/Downloads/Data";
char file_list[]="/home/ghost/file_list/";


//Global paramaters
struct sockaddr_in serv_addr, cli_addr;
socklen_t clilen;
bool  debug=true;

using namespace std;

// int sockfd_file;
struct sockaddr_in file_serv_addr;
struct hostent *fileserver;

//declarations
void receiveMessage(int);
void sendMessage(int);
int home_page(int);

//\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\
////////////////////////////////   Network Layer  \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\
//\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\

void error(const char *msg){
    cerr<<getpid() <<"  " <<msg << endl;
    perror(msg);
    exit(1);
}
//////////////////////////////File Sever Connection ////////////////////////////////////////////////

char msg_from_client[BUFFER_SIZE],msg_from_fs[BUFFER_SIZE];
int forwaded_bytes,fs_recv_bytes,bytes_read;

//Establish connection to server
int connectToFileServer(){
    int sockfd_file = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd_file < 0)
    error("ERROR opening socket");
    fileserver = gethostbyname(FILE_SERVER);
    if (fileserver == NULL) {
        fprintf(stderr,"ERROR,File server not found :: connectToFileServer\n");
        exit(0);
    }
    bzero((char *) &file_serv_addr, sizeof(file_serv_addr));
    file_serv_addr.sin_family = AF_INET;
    bcopy((char *)fileserver->h_addr,
    (char *)&file_serv_addr.sin_addr.s_addr,
    fileserver->h_length);
    file_serv_addr.sin_port = htons(FILE_SERVER_PORT);
    if (connect(sockfd_file,(struct sockaddr *) &file_serv_addr,sizeof(file_serv_addr)) < 0)
    error("ERROR connecting :: connectToFileServer");
    return sockfd_file;
}


void forwardMessage(int sockfd_file){
    forwaded_bytes = write(sockfd_file,msg_from_client,bytes_read);
    if ( debug) printf("\t----->%d %s %d\n",forwaded_bytes,"msg_to_fs", bytes_read);
    if (forwaded_bytes < 0)
        error("ERROR writing to socket :: forwardMessage");
}

void recvFileServerMessage(int sockfd_file){
    bzero(msg_from_fs,BUFFER_SIZE);
    fs_recv_bytes=0;
    while (fs_recv_bytes==0)
      fs_recv_bytes=read(sockfd_file,msg_from_fs,BUFFER_SIZE);
    if (fs_recv_bytes < 0){
        error("ERROR reading from socket :: recvFileServerMessage");
        close(sockfd_file);
    }
    if ( debug) printf("\t<-----%d %s \n",fs_recv_bytes,"msg_from_fs");
    if (fs_recv_bytes==0){
        printf("Server has gone away :: recvFileServerMessage \n" );
        close(sockfd_file);
    }
}




//forwards download request to file server
void start_proxy_server(int sockfd_file,int newsockfd){
    pid_t pid;
    pid = fork();
    while (waitpid(-1, 0, WNOHANG) > 0)//zombie handeling
      continue;
    if (pid!=0){ //parent process
        while (true){
            receiveMessage(newsockfd);
            forwardMessage(sockfd_file);
        }
    }
    else{ //child process
        prctl(PR_SET_PDEATHSIG,SIGTERM);
        while(true){
            recvFileServerMessage(sockfd_file);
            sendMessage(newsockfd);
        }
    }

}



//////////////////////////////Connection to Clients////////////////////////////////////////////////

//socket, bind, listen
int establishConenction(){
    printf("Starting server ....\n");
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
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
    label: if( debug)   printf("Server started... Waiting for connection...\n");
    for(;;){
        int newsockfd = accept(sockfd,
            (struct sockaddr *) &cli_addr,
            &clilen);
        if ( debug) printf("Client conneced :: Main process forked\n" );
        pid = fork();
        while (waitpid(-1, 0, WNOHANG) > 0)//zombie handeling
          continue;
        if (newsockfd < 0){
            error("ERROR on accept");
            close(newsockfd);
        }
        if(pid!=0){
            close(newsockfd);
            continue;
        }
        else{
            if( debug)  printf("Moving the process control for client :: Child %d\n" ,getpid());
            // connectToFileServer();
            return newsockfd;
        }
    }
    return -1;
}



void sendMessage(int newsockfd){
        int bytes_written = write(newsockfd,msg_from_fs,fs_recv_bytes);
        if ( debug) printf("<-----%d %s %d \n",bytes_written,"msg_to_client" ,bytes_written);
        if (bytes_written < 0)
            error("ERROR writing to socket");
}

void receiveMessage(int newsockfd){
    bzero(msg_from_client   ,BUFFER_SIZE);
    bytes_read=read(newsockfd,msg_from_client,BUFFER_SIZE);
    if ( debug) printf("----->%d %s\n",bytes_read,"msg_from_client");
    if (bytes_read < 0){
        error("ERROR reading from socket :: receiveMessage");
        close(newsockfd);
    }
    if(bytes_read==0){
        printf("Client has gone away..!! :: receiveMessage\n" );
        close(newsockfd);
    }
}

void sendResponseMessage(int newsockfd, char *response_message){
        int bytes_written = write(newsockfd,response_message,20);
        if( debug)  printf("<-----%d %s %d \n",bytes_written,response_message ,bytes_written);
        if (bytes_written < 0)
            error("ERROR writing to socket");
}


//\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\
////////////////////////////////   Authentication  \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\
//\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\


int home_page(int newsockfd){
    int sockfd_file=connectToFileServer();
    if( debug) printf("***** User Credentials ***** \n" );
    start_homepage: receiveMessage(newsockfd);
    char buffer[BUFFER_SIZE],response_message[BUFFER_SIZE];
    bzero(response_message,BUFFER_SIZE);
    snprintf(buffer,sizeof(buffer),"%s",msg_from_client);

    if( debug)  printf("Message Recv : %s\n",buffer);
    int choice=  atoi(strtok(buffer, ","));
    char *username= strtok(NULL, ",");
    char *password=strtok(NULL, ",");
    switch (choice) {
        case 1: if(usernameAvailable(username))
                    snprintf(response_message, sizeof(response_message),"true" );
                else
                    snprintf(response_message, sizeof(response_message),"false");
                break;
        case 2: if (!createUser(username,password))
                    snprintf(response_message, sizeof(response_message),"true");
                 else
                    snprintf(response_message, sizeof(response_message),"false");
                 break;
        case 3: if (authenticateUser(username,password)){
                    snprintf(response_message,sizeof(response_message),"true");
                    sendResponseMessage(newsockfd,response_message);
                    return sockfd_file;
                    }
                 else
                    snprintf(response_message, sizeof(response_message),"false");
                 break;

        case 0: snprintf(msg_from_client,sizeof(msg_from_client),"0, Client Exit");
                forwardMessage(sockfd_file);
                close(sockfd_file);
                close(newsockfd);
        default: if( debug)  printf("Root, We have a problem...!\n");
                  if( debug)  printf("%d %s %s\n",choice,username,password );
                  close(sockfd_file);
                  close(newsockfd);
    }
    if( debug)  printf("%s\n",response_message );
    sendResponseMessage(newsockfd,response_message);
    goto start_homepage;
    return -1;
}

//////////////////////////////////// Reserved for Main fun /////////////////////////////////////////////////

int main(int argc, char *argv[]){
  if(argc<4){
    fprintf(stderr,"usage %s hostname fileserver_port listen_port\n", argv[0] );
    exit(0);
  }
     snprintf(FILE_SERVER, sizeof(FILE_SERVER),"%s",argv[1]);
     FILE_SERVER_PORT=atoi(argv[2]);
     PORT=atoi(argv[3]);
     int newsockfd= establishConenction();
     int sockfd_file=home_page(newsockfd);
     printf("!!!!!!!!!!!!!!! The place of smokes  %d !!!!!!!!!!!!!!!\n" , getpid());
     start_proxy_server(sockfd_file,newsockfd);
     close(newsockfd);
     close(sockfd_file);
     return 0;
}
