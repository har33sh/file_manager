
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

#define HOST "10.129.23.200"
#define PORT 9511
#define BUFFER_SIZE 256

int sockfd, portno, n;
struct sockaddr_in serv_addr;
struct hostent *server;
char buffer[256],send_message[100];

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
    printf("---> %s\n",buffer );
}


//isolating socket modules from the other modules
int sendMessage(){
        printf("<----- : %s\n",send_message );
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




//upload file
int upload(){
    char filename[20],path[100];
    printf("Enter the path of the file : " );
    scanf("%s",path );
    printf("Save file as: ");
    scanf("%s",filename );
    snprintf(send_message, sizeof(send_message), "%s,%s", "4",filename);
    sendMessage();
    FILE *f;
    unsigned long fsize;
    f = fopen(filename, "r");
    if (f == NULL){
        printf("File not found!\n");
        return 1;
    }
    else{
        printf("Uploading the file......\n");
        while (!feof(f)){
            n=fread(buffer,sizeof(char), BUFFER_SIZE, f);
            int bytes_written = write(sockfd, buffer, n);
            }
        printf("%s File Successfully uploaded\n", filename);
    }
    fclose(f);
    return 0;
}


//needs to be checked if it works
void download(){
    printf("##### File List ####");
    char msg[]="send files";
    snprintf(send_message, sizeof(send_message), "%s,%s", "5",msg);
    printf("%s\n",buffer);
    //need to find way to get which file to download and download the file
    char filename[]="myfile";
    snprintf(send_message, sizeof(send_message), "%s,%s", "5",filename);
    sendMessage();
    FILE *fp;
    int n;
    fp = fopen(filename, "w");
    printf("Downloading... %s",filename);
    bzero(buffer,BUFFER_SIZE);
    while (n = read(sockfd,buffer,BUFFER_SIZE) > 0 ){
        n = fwrite(buffer, sizeof(char), sizeof(buffer), fp);
        }
    fclose(fp);
}


void closeConnection(){
    close(sockfd);
}


void reg(){
    bool availablity=false;
    char username[20],password[20];
    printf("Enter username : ");
    scanf("%s",username );
    snprintf(send_message, sizeof(send_message), "%s,%s", "1",username);
    sendMessage();
    // availablity=bool(recieveMessage());
    while(!availablity){
        printf("%s already taken, try a differnt one...\n Enter username :",username );
        scanf("%s",username );
        snprintf(send_message, sizeof(send_message), "%s,%s", "1",username);
        sendMessage();
        // availablity=sendMessage(1,username);
    }
    printf("Enter password :");
    scanf("%s",password );
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
        }
        else
            return auth;
    }
    return auth;
}

//later modify with private file storage //changet the return type of the login to username if the auth is Success
void file_menu(){
    int choice;
    printf("1.Upload\n2.Download\n3.logout\nEnter your choice : ");
    scanf("%d",&choice);
    switch (choice) {
        case 1: upload(); break;
        case 2: download(); break;
        case 3: return;
        default: printf("Invalid Option selected..!! \n");
    }
}

void menu(){
    snprintf(send_message, sizeof(send_message), "%s,%s", "1","username");
    printf("%s\n",send_message );
    int choice;
    printf("______________________________________________FILE SERVER______________________________________________\n" );
    while (true) {
        printf("1. Register \n2.Login\n3.Exit\nEnter your Choice : ");
        scanf("%d",&choice);

        switch (choice) {
            case 1: reg(); break;
            case 2: if(login()) file_menu(); break;
            case 3: return;
            default: printf("Invalid Option selected..!! \n");
        }
    }
}

int main(int argc, char *argv[]){
    establishConenction();
    // while(true){
    //     printf("Please enter the message: ");
    //     bzero(buffer,256);
    //     fgets(buffer,255,stdin);
    //     int response=sendMessage(buffer);
    //     printf("Response code %d \n",response);
    // }
    // upload();
    menu();
    closeConnection();
    return 0;
}
