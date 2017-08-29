// #include<iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include<dirent.h>


#define PORT 9555
#define BUFFER_SIZE 256

//Global paramaters
int sockfd, newsockfd, portno;
socklen_t clilen;
char buffer[256];
char response_message[100];
struct sockaddr_in serv_addr, cli_addr;
char file_dir[]="/home/ghost/Downloads";
char file_list[]="/home/ghost/file_list.txt";

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

void force_send_response(){ //Clients waits for message, can be used to send dummy messges
    snprintf(response_message, sizeof(response_message), "Let the force be with you");
    int _=write(newsockfd,response_message,sizeof(response_message));
    printf("Force Message : %s\n",response_message );
}

void send_file_list(){
    //need to update the file lets call it update_file_avail()
    printf("Sending the file list ......\n");
    FILE *f;
    unsigned long fsize;
    bzero(buffer,BUFFER_SIZE);
    f = fopen(file_list, "rb");
    bzero(buffer,BUFFER_SIZE);
    while (!(feof(f))){
        int n=fread(buffer,sizeof(char), BUFFER_SIZE, f);
        int bytes_written = write(newsockfd, buffer, n);
        bzero(buffer,BUFFER_SIZE);
    }
    printf(" File list Successfully send\n");
    bzero(buffer,BUFFER_SIZE);
    fclose(f);

}
////////////////////////////////////////////////////////




void fileRecieve(char *filename){
    printf("Receiving... %s\n",filename);
    FILE *fp;
    int n;
    fp = fopen(filename, "wb");
    bzero(buffer,BUFFER_SIZE);
    n = read(newsockfd,buffer,BUFFER_SIZE) ;
    while (n == BUFFER_SIZE ){
        int x =fwrite(buffer, sizeof(char), sizeof(buffer), fp);
        bzero(buffer,BUFFER_SIZE);
        n = read(newsockfd,buffer,BUFFER_SIZE) ;
        // printf("===> %d  %d\n",x,n );
        }
    int x =fwrite(buffer, sizeof(char), sizeof(buffer), fp);
    bzero(buffer,BUFFER_SIZE);
    // printf("=cc==> %d  %d\n",1,n );
    fclose(fp);
    printf("%s Successfully received\n",filename );
    snprintf(response_message, sizeof(response_message), "Received Successfully");
}

void closeConnection(){
    close(newsockfd);
    close(sockfd);
    printf("Closed all connections.." );
}



void fileSend(){
    send_file_list();
    printf("Client Downloading file\n" );
    int x =read(newsockfd,buffer,255);
    int choice=  atoi(strtok(buffer, ","));
    printf("input %d\n",choice );



    FILE *f;
    char filename[]="/home/ghost/SCC305.png";
    f = fopen(filename, "rb");
    unsigned long fsize;
    bzero(buffer,BUFFER_SIZE);
    printf("Sending the file......\n");
    bzero(buffer,BUFFER_SIZE);
    int n=fread(buffer,sizeof(char), BUFFER_SIZE, f);
    while (n>0){
        int bytes_written = write(newsockfd, buffer, n);
        bzero(buffer,BUFFER_SIZE);
        n=fread(buffer,sizeof(char), BUFFER_SIZE, f);
    }
    printf("%s File Send Successfully uploaded\n", filename);

    bzero(buffer,BUFFER_SIZE);
    fclose(f);


}

//process message and sent response
void onMessage(char *buffers){
    printf("Message Recv : %s\n",buffer);
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
        case 4: force_send_response();fileRecieve(msg1); break;
        case 5: fileSend();break;
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
            printf("Message Send: %s\n",response_message );
            if (n < 0) error("ERROR writing to socket");
        }
}



int main(int argc, char *argv[]){
     establishConenction();
     fileSend();
    //  onRecieve();
     closeConnection();
     return 0;
}
