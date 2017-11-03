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
// #define FILE_SERVER "10.129.23.200"
// #define FILE_SERVER_PORT 9334
// #define PORT 4334
bool  debug=true,show_msg=false;
char file_list[]="/home/ghost/file_list/";
char file_dir[]="/home/ghost/Downloads/Data";
int FILE_SERVER_PORT,PORT;
char FILE_SERVER[50];
#define BUFFER_SIZE 256


//Global paramaters
int sockfd, newsockfd, portno;
struct sockaddr_in serv_addr, cli_addr;
socklen_t clilen;
char send_msg[BUFFER_SIZE],response_message[BUFFER_SIZE];
char buffer[BUFFER_SIZE],auth_user[100];

using namespace std;

int sockfd_file;
struct sockaddr_in file_serv_addr;
struct hostent *fileserver;

//declarations
void receiveMessage();
void sendMessage();
void home_page();
void closeConnection();

//\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\
////////////////////////////////   Network Layer  \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\
//\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\

void error(const char *msg){
cerr<<getpid() <<"  " <<msg << endl;
perror(msg);
exit(1);
}

int send_message(int socket_id,const char *message,int length){
  int bytes_written = write(socket_id,message,length);
  if (debug)  cout<<"<----" <<(bytes_written==length);
  if(show_msg)  cout<< " Message : "<<message<<endl;
  return bytes_written;
}

int recv_message(int socket_id,char *message){
  bzero(message,BUFFER_SIZE);
  int bytes_read=read(socket_id,message,BUFFER_SIZE);
  if (debug) cout<<"----->"<<bytes_read;
  if(show_msg) cout<< "Message :"<< message<<endl;
  if (bytes_read <=0){
      error("ERROR reading from socket :: receiveMessage");
  }
  return bytes_read;
}

//////////////////////////////File Sever Connection ////////////////////////////////////////////////

char msg_from_client[BUFFER_SIZE],msg_from_fs[BUFFER_SIZE];
int forwaded_bytes,fs_recv_bytes,bytes_read;

//Establish connection to server
void connectToFileServer(){
    sockfd_file = socket(AF_INET, SOCK_STREAM, 0);
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
}


void forwardMessage(){
    forwaded_bytes = write(sockfd_file,msg_from_client,bytes_read);
    if ( debug) printf("\t----->%d %s %d\n",forwaded_bytes,"msg_to_fs", bytes_read);
    if (forwaded_bytes < 0)
        error("ERROR writing to socket :: forwardMessage");
}

void recvFileServerMessage(){
    bzero(msg_from_fs,BUFFER_SIZE);
    fs_recv_bytes=0;
    while (fs_recv_bytes==0)
      fs_recv_bytes=read(sockfd_file,msg_from_fs,BUFFER_SIZE);
    if (fs_recv_bytes < 0){
        error("ERROR reading from socket :: recvFileServerMessage");
        closeConnection();
    }
    if ( debug) printf("\t<-----%d %s \n",fs_recv_bytes,"msg_from_fs");
    if (fs_recv_bytes==0){
        printf("Server has gone away :: recvFileServerMessage \n" );
        closeConnection();
    }
}



void forw(){
  while(true){
    receiveMessage();
    forwardMessage();
  }
}
void rev(){
  while(true){
    recvFileServerMessage();
    sendMessage();
  }
}
//forwards download request to file server
void start_proxy_server(){
  pid_t pid;
  pthread_t forward,reverse;
  if(pthread_create(forward, NULL, forw, (void *)0)==0){
    printf("11111111\n" );
    forw();
  }
  else{
    printf("2222222222\n" );
    rev();
  }
    // { //parent process
    //     while (true){
    //       char temp[BUFFER_SIZE];
    //       // int len =recv_message(newsockfd,temp);
    //       // send_message(sockfd,temp,len);
    //
    //     }
    // }
    // else{ //child process
    //   if(pthread_create(reverse, NULL, start_threads, (void *)i)==0)
    //   while(true){
    //       // recv_message()
    //
    //     }
    // }

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
    label: if( debug)   printf("Server started... Waiting for connection...\n");
    for(;;){
        newsockfd = accept(sockfd,
            (struct sockaddr *) &cli_addr,
            &clilen);
        if ( debug) printf("Client conneced :: Main process forked\n" );
        pid = fork();
        while (waitpid(-1, 0, WNOHANG) > 0)//zombie handeling
          continue;
        if (newsockfd < 0){
            error("ERROR on accept");
            closeConnection();
        }
        if(pid!=0){
            close(newsockfd);
            continue;
        }
        else{
            if( debug)  printf("Moving the process control for client :: Child %d\n" ,getpid());
            connectToFileServer();
            break;
        }
    }

}


void closeConnection(){ //closes connection with client and proxy server
    if ( debug) printf("Closing Connections\n" );
    snprintf(send_msg,sizeof(send_msg),"0,Exiting");
    try{
        // write(sockfd,send_msg,sizeof(send_msg)); //causes problem
        close(sockfd);
        if ( debug) printf("Closed File Server Connection \n" );
    }
    catch(int e){
        if ( debug) printf("File server was closed before\n" );
    }

    try{
        write(newsockfd,send_msg,sizeof(send_msg));
        close(newsockfd);
        if ( debug) printf("Closed Client Connection  \n" );
    }
    catch(int e){
        printf("Client is already closed\n" );
    }

    printf("Closed all connections.. killed process %d\n",getpid() );
    // signal(SIGINT,closeConnection); //trying to kill children if any
    // kill(0,SIGTERM);
    exit(0);
}



void sendMessage(){
        int bytes_written = write(newsockfd,msg_from_fs,fs_recv_bytes);
        if ( debug) printf("<-----%d %s %d \n",bytes_written,"msg_to_client" ,bytes_written);
        if (bytes_written < 0)
            error("ERROR writing to socket");
}

void receiveMessage(){
    bzero(msg_from_client   ,BUFFER_SIZE);
    bytes_read=read(newsockfd,msg_from_client,BUFFER_SIZE);
    if ( debug) printf("----->%d %s\n",bytes_read,"msg_from_client");
    if (bytes_read < 0){
        error("ERROR reading from socket :: receiveMessage");
        closeConnection();
    }
    if(bytes_read==0){
        printf("Client has gone away..!! :: receiveMessage\n" );
        closeConnection();
    }
}

void sendResponseMessage(){
        int bytes_written = write(newsockfd,response_message,20);
        if( debug)  printf("<-----%d %s %d \n",bytes_written,response_message ,bytes_written);
        if (bytes_written < 0)
            error("ERROR writing to socket");
}


//\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\
////////////////////////////////   Authentication  \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\
//\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\


void home_page(){
    if( debug) printf("***** User Credentials ***** \n" );
    start_homepage: receiveMessage();
    char buffer[BUFFER_SIZE];
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
                    sendResponseMessage();
                    return ;
                    }
                 else
                    snprintf(response_message, sizeof(response_message),"false");
                 break;

        case 0: snprintf(msg_from_client,sizeof(msg_from_client),"0, Client Exit");forwardMessage(); closeConnection();
        default: if( debug)  printf("Root, We have a problem...!\n");
                  if( debug)  printf("%d %s %s\n",choice,username,password );
                 closeConnection();
    }
    if( debug)  printf("%s\n",response_message );
    sendResponseMessage();
    goto start_homepage;
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
     establishConenction();
     home_page();
     printf("!!!!!!!!!!!!!!! The place of smokes  %d !!!!!!!!!!!!!!!\n" , getpid());
     start_proxy_server();
     closeConnection();
     return 0;
}
