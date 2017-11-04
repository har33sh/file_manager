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
bool  debug=true;
#define BUFFER_SIZE 256


//Global paramaters
int FILE_SERVER_PORT,PORT;
char FILE_SERVER[50];
struct sockaddr_in serv_addr, cli_addr;
char file_dir[]="/home/ghost/Downloads/Data";
char file_list[]="/home/ghost/file_list/";
socklen_t clilen;
struct sockaddr_in file_serv_addr;
struct hostent *fileserver;

using namespace std;

//declarations
void recv_message(int,char*);
void send_message(int,char*);
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


struct args{
 int sock_cli;
 int sock_file;
};

void *client_to_server(void *a){
  struct args *arg=(struct args*)a;
  int sock_file=arg->sock_file;
  int sock_cli=arg->sock_cli;
  char buffer[BUFFER_SIZE];
  while(true){
    recv_message(sock_cli,buffer);
    send_message(sock_file,buffer);
  }
}
void *server_to_client(void *a){
  struct args *arg=(struct args*)a;
  int sock_file=arg->sock_file;
  int sock_cli=arg->sock_cli;
  char buffer[BUFFER_SIZE];
  while(true){
    recv_message(sock_file,buffer);
    send_message(sock_cli,buffer);
  }
}


//forwards download request to file server
void start_proxy_server(int sockfd_file,int newsockfd){
  pthread_t frwd,rev;
  struct args *a1;
  a1=(args*)malloc(sizeof(struct args));
  a1->sock_file=sockfd_file;
  a1->sock_cli=newsockfd;
  pthread_create(&frwd,NULL,client_to_server,a1);
  pthread_create(&rev,NULL,server_to_client,a1);
  pthread_join(frwd,NULL);
  pthread_join(rev,NULL);
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
            return newsockfd;
        }
    }
    return -1;
}



void send_message(int newsockfd,char *msg_from_fs){
        int bytes_written = write(newsockfd,msg_from_fs,strlen(msg_from_fs));
        if ( debug) printf("<-----%d %s %d \n",bytes_written,"msg_to_client" ,bytes_written);
        if (bytes_written < 0)
            error("ERROR writing to socket");
}

void recv_message(int newsockfd,char *msg_from_client){
    bzero(msg_from_client   ,BUFFER_SIZE);
    int bytes_read=read(newsockfd,msg_from_client,BUFFER_SIZE);
    if ( debug) printf("----->%d %s\n",bytes_read,"msg_from_client");
    if (bytes_read < 0){
        error("ERROR reading from socket :: recv_message");
        close(newsockfd);
    }
    if(bytes_read==0){
        printf("Client has gone away..!! :: recv_message\n" );
        close(newsockfd);
    }
}



//\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\
////////////////////////////////   Authentication  \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\
//\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\


int home_page(int newsockfd){
    int sockfd_file=connectToFileServer();
    if( debug) printf("***** User Credentials ***** \n" );
    char buffer[BUFFER_SIZE],response_message[BUFFER_SIZE], msg_from_client[BUFFER_SIZE];
    bzero(response_message,BUFFER_SIZE);
    start_homepage: recv_message(newsockfd,msg_from_client);
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
                    send_message(newsockfd,response_message);
                    return sockfd_file;
                    }
                 else
                    snprintf(response_message, sizeof(response_message),"false");
                 break;

        case 0: snprintf(msg_from_client,sizeof(msg_from_client),"0, Client Exit");
                send_message(sockfd_file,msg_from_client);
                close(sockfd_file);
                close(newsockfd);
        default: if( debug)  printf("Root, We have a problem...!\n");
                  if( debug)  printf("%d %s %s\n",choice,username,password );
                  close(sockfd_file);
                  close(newsockfd);
    }
    if( debug)  printf("%s\n",response_message );
    send_message(newsockfd,response_message);
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
