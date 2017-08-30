
#include <stdio.h>
#include <iostream>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
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

bool debug =true;


//\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\
////////////////////////////////   Network Layer\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\
//\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\

#define HOST "10.129.23.200"
#define PORT 9555
#define BUFFER_SIZE 256

int sockfd, portno, n;
struct sockaddr_in serv_addr;
struct hostent *server;
char buffer[256],send_message[100],received_message[100],file_names[100];

void error(const char *msg){
    perror(msg);
    exit(0);
}

//Establish connection to server
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


void onMessage(char *buffer){
    if (debug)
        printf("-----> %s\n",buffer );
    snprintf(received_message,sizeof(received_message),"%s",buffer);
}


//isolating socket modules from the other modules
int sendMessage(){
        if (debug)
            printf("<-----  %s\n",send_message );
        n = write(sockfd,send_message,strlen(send_message));
        if (n < 0)
        error("ERROR writing to socket");
        bzero(buffer,256);
        n = read(sockfd,buffer,255);
        onMessage(buffer);
        if (n < 0)
        error("ERROR reading from socket");
        return 1;
}


void closeConnection(){
    snprintf(send_message, sizeof(send_message), "%s,%s", "0","Byee..");
    int n = write(sockfd,send_message,strlen(send_message));
    close(sockfd);
    printf("//////////////////////////////////////////////////////////////////////////////\n");
    printf("///////////////////////////        Thank You           ///////////////////////\n");
    printf("//////////////////////////////////////////////////////////////////////////////\n");
}




//\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\
//////////////////////////////// Data Access Layer///////////////////////////////////////////////
//\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\

//upload file
int upload(){
    char filename[20],save_as[100];
    printf("Enter the path of the file : " );
    scanf("%s",filename );
    printf("Save file as: ");
    scanf("%s",save_as );
    snprintf(send_message, sizeof(send_message), "%s,%s", "4",save_as);
    sendMessage();
    FILE *f;
    unsigned long fsize;
    bzero(buffer,BUFFER_SIZE);
    f = fopen(filename, "rb");
    if (f == NULL){
        printf("%s File not found!\n",filename);
        return 1;
    }
    else{
        printf("Uploading the file......\n");
        bzero(buffer,BUFFER_SIZE);
        n=fread(buffer,sizeof(char), BUFFER_SIZE, f);
        while (n>0){
            int bytes_written = write(sockfd, buffer, n);
            bzero(buffer,BUFFER_SIZE);
            n=fread(buffer,sizeof(char), BUFFER_SIZE, f);
        }
        printf("%s File Successfully uploaded\n", filename);
    }
    bzero(buffer,BUFFER_SIZE);
    fclose(f);
    return 0;
}



void load_file_list(){
    snprintf(send_message, sizeof(send_message), "%s,%s", "5","Send me the file_list");
    int bytes_writen = write(sockfd,send_message,strlen(send_message));
    if(debug)
        printf("Receiving file_list... \n");
    FILE *fp;
    int n;
    fp = fopen(".file_list.txt", "wb");
    bzero(buffer,BUFFER_SIZE);
    n = read(sockfd,buffer,BUFFER_SIZE) ;
    while (n == BUFFER_SIZE ){
        int x =fwrite(buffer, sizeof(char), sizeof(buffer), fp);
        bzero(buffer,BUFFER_SIZE);
        n = read(sockfd,buffer,BUFFER_SIZE) ;
    }
    int x =fwrite(buffer, sizeof(char), sizeof(buffer), fp);
    bzero(buffer,BUFFER_SIZE);
    // printf("=cc==> %d  %d\n",1,n );
    fclose(fp);
    if(debug)
        printf("Successfully received file list\n");
    printf("\n##### File List ####\n");
    char c;
    fp = fopen(".file_list.txt", "r");
    while ((c=fgetc(fp))!=EOF)
      printf("%c",c );
    fclose(fp);
    printf("\n####################\n");
}

//needs to be checked if it works
void download(){
    load_file_list();
    int choice;
    char save_file_as[100];
    printf("Enter the file number to download :" );
    scanf(" %d",&choice );
    printf("Save file as :" );
    scanf("%s",save_file_as);
    printf("Downloading... %s\n",save_file_as);
    snprintf(send_message, sizeof(send_message), "%d",choice);
    int bytes_written = write(sockfd,send_message,strlen(send_message));
    FILE *fp;
    int n;
    fp = fopen(save_file_as, "wb");
    bzero(buffer,BUFFER_SIZE);
    n = read(sockfd,buffer,BUFFER_SIZE) ;
    while (n==BUFFER_SIZE ){
        n = fwrite(buffer, sizeof(char), sizeof(buffer), fp);
        bzero(buffer,BUFFER_SIZE);
        n = read(sockfd,buffer,BUFFER_SIZE) ;
        }
    n = fwrite(buffer, sizeof(char), sizeof(buffer), fp);
    bzero(buffer,BUFFER_SIZE);
    fclose(fp);
    printf("Downloaded %s\n",save_file_as );
}

//\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\
//////////////////////////////// Presentation Layer///////////////////////////////////////////////
//\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\

void reg(){
    bool availablity=false;
    char username[20],password[20];
    printf("Enter username : ");
    scanf("%s",username );
    snprintf(send_message, sizeof(send_message), "%s,%s", "1",username);
    sendMessage();
    if(received_message[0]=='t')
      availablity=true;
    printf("==>%s %d\n",received_message,availablity );
    while(!availablity){
        printf("%s already taken, try a differnt one...\n Enter username :",username );
        scanf("%s",username );
        snprintf(send_message, sizeof(send_message), "%s,%s", "1",username);
        sendMessage();
        if(received_message[0]=='t')
          availablity=true;
    }
    printf("Enter password :");
    scanf("%s",password );
    printf("%s registered Successfully\n",username );
    snprintf(send_message, sizeof(send_message), "%s,%s,%s", "2",username,password);
    sendMessage();
}

bool login(){
    bool auth=false;
    char yes_or_no[2];
    char username[20],password[20];
    printf("Enter username : ");
    scanf("%s",username );
    printf("Enter password : ");
    scanf("%s",password );
    snprintf(send_message, sizeof(send_message), "%s,%s,%s", "3",username,password);
    sendMessage();
    if(received_message[0]=='t')
      auth=true;
    while(!auth){
        printf("Bad username or password \nTry again ? [Y/n] : ");
        scanf("%s",yes_or_no);
        if (yes_or_no[0]=='y' or yes_or_no[0]=='Y'){
            printf("Enter username : ");
            scanf("%s",username );
            printf("Enter password : ");
            scanf("%s",password );
            snprintf(send_message, sizeof(send_message), "%s,%s,%s", "3",username,password);
            sendMessage();
            if(received_message[0]=='t')
              auth=true;
        }
        else
            return auth;
    }
    printf("### user %s Successfully Authenticated  ###\n",username );
    return auth;
}

//later modify with private file storage //changet the return type of the login to username if the auth is Success
void file_menu(){
    int choice;
    while (true) {
        printf("1. Upload\n2. Download\n3. Logout\nEnter your choice : ");
        scanf("%d",&choice);
        switch (choice) {
            case 1: upload(); break;
            case 2: download(); break;
            case 3: return;
            default: printf("Invalid Option selected..!! \n");
        }
    }
}

void menu(){
    snprintf(send_message, sizeof(send_message), "%s,%s", "1","username");
    printf("%s\n",send_message );
    int choice;
    printf("//////////////////////////////////////////////////////////////////////////////\n");
    printf("///////////////////////////        FILE SERVER         ///////////////////////\n");
    printf("//////////////////////////////////////////////////////////////////////////////\n");
    while (true) {
        printf("1. Register \n2. Login\n3. Exit\nEnter your Choice : ");
        scanf("%d",&choice);
        switch (choice) {
            case 1: reg(); break;
            case 2: if(login()) file_menu(); break;
            case 3: closeConnection();exit(0);
            default: printf("Invalid Option selected..!! \n");
        }
    }
}



//////////////////////////////// MAIN

int main(int argc, char *argv[]){
    establishConenction();
    menu();
    closeConnection();
    return 0;
}
