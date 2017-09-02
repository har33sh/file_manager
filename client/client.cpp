
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


//Config
bool debug =false;
#define HOST "10.129.23.200"
// #define PORT 8565
#define BUFFER_SIZE 256

int PORT ;
//Global Variables (Shared with all layers)
int sockfd, portno, n;
struct sockaddr_in serv_addr;
struct hostent *server;
char file_buffer[BUFFER_SIZE],send_message[BUFFER_SIZE],response_message[BUFFER_SIZE],file_names[BUFFER_SIZE];
char username[20];
//\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\
////////////////////////////////   Network Layer  \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\
//\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\

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


void sendMessage(){
        int bytes_written = write(sockfd,send_message,strlen(send_message));
        if(debug)
            printf("<-----%d %s\n",bytes_written,send_message);
        if (bytes_written < 0)
            error("ERROR writing to socket");

}

void receiveMessage(){
    bzero(response_message,BUFFER_SIZE);
    int bytes_read = 0;
    while(bytes_read==0)
        bytes_read=read(sockfd,response_message,BUFFER_SIZE);
    if (debug)
        printf("----->%d %s\n",bytes_read,response_message);
    if (bytes_read < 0)
        error("ERROR reading from socket");
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
    snprintf(send_message, sizeof(send_message),"%d,%s", 4,username);
    sendMessage();
    receiveMessage(); //ACK
    char filename[20],save_as[100];
    printf("Enter the path of the file : " );
    scanf("%s",filename );
    printf("Save file as: ");
    scanf("%s",save_as );
    int fsize,bytes_read,bytes_written,bytes_left;
    bzero(file_buffer,BUFFER_SIZE);
    FILE *f= fopen(filename, "rb");
    if (f == NULL){
        printf("%s File not found!\n",filename);
        return 1;
    }
    else{
        fseek(f, 0, SEEK_END);
        fsize = ftell(f);
        rewind(f);
        snprintf(send_message, sizeof(send_message), "%d,%s", fsize,save_as);
        sendMessage();
        bytes_left=fsize;
        printf("FileSize : %d  FileName : %s \nUploading the file...... \n",fsize,filename);
        receiveMessage();
        while (bytes_left>0){
            bytes_read = fread(file_buffer,sizeof(char), BUFFER_SIZE, f);
            bytes_written = write(sockfd, file_buffer, bytes_read);
            bytes_left-=bytes_written;
            // printf("%s\n",file_buffer );
            if (debug)
                printf("<----Uploaded %d of %d\n",(fsize-bytes_left),fsize );
            receiveMessage();
        }
        printf("%s File Successfully uploaded\n", filename);
    }
    bzero(file_buffer,BUFFER_SIZE);
    fclose(f);
    return 0;
}


int min(int x,int y) {
    if(x>y)
        return x;
    return y;
}

void load_file_list(){
    receiveMessage();
    int filesize=  atoi(strtok(response_message, ","));
    char *filename= strtok(NULL, ",");
    if(debug)
        printf("Receiving File List: %s FileSize: %d\n",filename,filesize);
    int bytes_left,bytes_read,bytes_written;
    snprintf(send_message, sizeof(send_message), "%s","Time to download");
    sendMessage();
    FILE *fp = fopen(".file_list.txt", "wb");
    bytes_left=filesize;
    bzero(file_buffer,BUFFER_SIZE);
    while (bytes_left>0){
        bytes_read = read(sockfd,file_buffer,min(BUFFER_SIZE,bytes_left)) ;
        bytes_written=fwrite(file_buffer, sizeof(char), bytes_read, fp);
        bytes_left-=bytes_written;
        if (debug)
            printf("Receiving %d of %d\n",(filesize-bytes_left),filesize );
    }
    bzero(file_buffer,BUFFER_SIZE);
    fclose(fp);
    if(debug)
        printf("Received File List FileSize: %d Received %d\n",(filesize-bytes_left),filesize);
    char c;
    fp = fopen(".file_list.txt", "r");
    printf("\n##### File List ####\n");
    while ((c=fgetc(fp))!=EOF)
      printf("%c",c );
    printf("\n####################\n");
    fclose(fp);
}

//needs to be checked if it works
void download(){
    snprintf(send_message, sizeof(send_message),"%d,%s", 5,username);
    sendMessage();
    load_file_list();
    int choice;
    char save_file[100];
    printf("Enter the file number to download :" );
    scanf(" %d",&choice );
    printf("Save file as :" );
    scanf("%s",save_file);
    printf("Downloading... %s\n",save_file);
    snprintf(send_message, sizeof(send_message), "%d,",choice);
    sendMessage(); //sending the choice
    if (debug)
        printf("waiting for file details\n" );
    receiveMessage(); //gets the file details
    int filesize=  atoi(strtok(response_message, ","));
    int bytes_left,bytes_read,bytes_written;
    FILE *fp = fopen(save_file, "wb");
    bytes_left=filesize;
    bzero(file_buffer,BUFFER_SIZE);
    snprintf(send_message, sizeof(send_message), "%s,","Ready for download");
    sendMessage(); //starting Download
    while (bytes_left>0){
        bytes_read = read(sockfd,file_buffer,min(BUFFER_SIZE,bytes_left)) ;
        snprintf(send_message, sizeof(send_message),"Ack  %d of %d",(filesize-bytes_left),filesize );
        sendMessage(); //starting Download
        if(bytes_read==-1){
            printf("%s\n", "Socket cant read");
        }
        bytes_written=fwrite(file_buffer, sizeof(char), bytes_read, fp);
        bytes_left-=bytes_written;
        if (debug)
            printf("%d Receiving %d of %d\n",bytes_written,(filesize-bytes_left),filesize );
    }
    bzero(file_buffer,BUFFER_SIZE);
    fclose(fp);
    // receiveMessage(); //Server sends a completion flag from the whil true loop
    printf("Download File: %s filesize: %d\n",save_file,filesize );
}

//\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\
//////////////////////////////// Presentation Layer///////////////////////////////////////////////
//\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\

void reg(){
    bool availablity=false;
    char password[20];
    printf("Enter username : ");
    scanf("%s",username );
    snprintf(send_message, sizeof(send_message), "%s,%s", "1",username);
    sendMessage();
    receiveMessage();
    if(response_message[0]=='t')
      availablity=true;
    while(!availablity){
        printf("%s already taken, try a differnt one...\n Enter username :",username );
        scanf("%s",username );
        snprintf(send_message, sizeof(send_message), "%s,%s", "1",username);
        sendMessage();
        receiveMessage();
        if(response_message[0]=='t')
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
    char password[20];
    printf("Enter username : ");
    scanf("%s",username );
    printf("Enter password : ");
    scanf("%s",password );
    snprintf(send_message, sizeof(send_message), "%s,%s,%s", "3",username,password);
    sendMessage();
    receiveMessage();
    if(response_message[0]=='t')
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
            receiveMessage();
            if(response_message[0]=='t')
              auth=true;
        }
        else
            return auth;
    }
    printf("### user %s Successfully Authenticated  ###\n",username );
    return auth;
}

void logout(){
  snprintf(send_message, sizeof(send_message), "%s", "0,Client logging out");
  sendMessage();
}


//later modify with private file storage --> If needed
void file_menu(){
    int choice;
    while (true) {
        printf("1. Upload\n2. Download\n3. Logout\nEnter your choice : ");
        scanf("%d",&choice);
        switch (choice) {
            case 1: upload(); break;
            case 2: download(); break;
            case 3: logout();return;
            default: printf("Invalid Option selected..!! \n");
        }
    }
}

void menu(){
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
    scanf("%d",&PORT );
    PORT=PORT+1;
    establishConenction();
    menu();
    // file_menu();
    closeConnection();
    return 0;
}
