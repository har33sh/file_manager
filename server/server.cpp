// #include<iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include<dirent.h>
#include <string>
#include "db.h"
using namespace std;

//config
#define PORT 9540
#define BUFFER_SIZE 256
char file_dir[]="/home/ghost/Downloads/Data";
char file_list[]="/home/ghost/file_list.txt";

//Global paramaters
int sockfd, newsockfd, portno;
struct sockaddr_in serv_addr, cli_addr;
socklen_t clilen;
char file_buffer[BUFFER_SIZE],send_message[BUFFER_SIZE],response_message[BUFFER_SIZE],file_names[100][100];
char buffer[256],auth_user[100];
using namespace std;



//\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\
////////////////////////////////   Network Layer  \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\
//\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\


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
        else
            break;
    }

}


void closeConnection(){
    close(newsockfd);
    close(sockfd);
    printf("Closed all connections.." );
}



void sendMessage(){
        int bytes_written = write(newsockfd,send_message,strlen(send_message));
        printf("<-----%d %s\n",bytes_written,send_message);
        if (bytes_written < 0)
            error("ERROR writing to socket");
}

void receiveMessage(){
    bzero(response_message,BUFFER_SIZE);
    int bytes_read = read(newsockfd,response_message,BUFFER_SIZE);
    printf("----->%d %s\n",bytes_read,response_message);
    if (bytes_read < 0)
        error("ERROR reading from socket");
}



//\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\
//////////////////////////////// Data Access Layer///////////////////////////////////////////////
//\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\


void fileRecieve(){
    printf("======== File Receiving =========\n");
    FILE *fp;
    int bytes_left,bytes_read,bytes_written;

    receiveMessage();
    int filesize=  atoi(strtok(response_message, ","));
    char *filename= strtok(NULL, ",");
    printf("Receiving File: %s FileSize: %d\n",filename,filesize);

    char file_location[100];
    snprintf(file_location,sizeof(file_location),"%s/%s/%s",file_dir,auth_user,filename);
    fp = fopen(file_location, "wb");
    snprintf(send_message, sizeof(send_message), "%s","Ready to recive");
    sendMessage();
    bzero(file_buffer,BUFFER_SIZE);
    bytes_left=filesize;
    while (bytes_left>0){
        bytes_read = read(newsockfd,file_buffer,min(BUFFER_SIZE,bytes_left)) ;
        bytes_written=fwrite(file_buffer, sizeof(char), bytes_read, fp);
        bytes_left-=bytes_written;
        printf("Receiving %d of %d\n",(filesize-bytes_left),filesize );
        }
    bzero(file_buffer,BUFFER_SIZE);
    fclose(fp);
    printf("Received File: %s Received: %d  Wrote: %d \n",filename,(filesize-bytes_left),filesize );
    snprintf(response_message, sizeof(response_message), "Received Successfully");
}


//read the files in the entire directory, saves it in a array and saves it in file_list
void update_file_list(){
    DIR *d;
    struct dirent *dir;
    d = opendir(file_dir);
    FILE *f;
    f=fopen(file_list,"w");
    int i=1;
    while ((dir = readdir(d)) != NULL){
      snprintf(file_names[i],sizeof(file_names[i]),"%s",dir->d_name);
      fprintf(f, "%d : %s\n",i,file_names[i] );
      i++;
    }
    closedir(d);
    fclose(f);

}

void send_file_list(){
    update_file_list();
    FILE *f= fopen(file_list, "rb");
    fseek(f, 0, SEEK_END);
    int filesize = ftell(f);
    rewind(f);
    snprintf(send_message, sizeof(send_message), "%d,%s", filesize,"file_list");
    sendMessage();
    receiveMessage(); //For Sync
    int bytes_read,bytes_written,bytes_left;
    bytes_left=filesize;
    printf("Sending the file list ...... Size %d\n",filesize);
    bzero(file_buffer,BUFFER_SIZE);
    while (bytes_left>0){
        bytes_read = fread(file_buffer,sizeof(char), BUFFER_SIZE, f);
        bytes_written = write(newsockfd, file_buffer, bytes_read);
        bytes_left-=bytes_written;
    }
    printf("File list Successfully\n");
    bzero(file_buffer,BUFFER_SIZE);
    fclose(f);
}


void fileSend(){
    printf("======== File Sending =========\n");
    send_file_list();
    receiveMessage(); //get the choice from client
    int choice=  atoi(strtok(response_message, ","));
    printf("input %d\n",choice );
    char filename[100];
    snprintf(filename,sizeof(filename),"%s/%s",file_dir,file_names[choice]); //file abs address
    int fsize,bytes_read,bytes_written,bytes_left;
    bzero(file_buffer,BUFFER_SIZE);
    FILE *f= fopen(filename, "rb");
    if (f == NULL)
        printf("%s File not found!\n",filename);

    else{
        fseek(f, 0, SEEK_END);
        fsize = ftell(f);
        rewind(f);
        snprintf(send_message, sizeof(send_message), "%d,%s", fsize,file_names[choice]);
        sendMessage();//sending the file details

        bytes_left=fsize;
        printf("Requesting the file FileSize : %d  FileName : %s \n",fsize,filename);
        receiveMessage(); //let it wait
        while (bytes_left>0){
            bytes_read = fread(file_buffer,sizeof(char), min(BUFFER_SIZE,bytes_left), f);
            bytes_written = write(newsockfd, file_buffer, bytes_read);
            receiveMessage();
            bytes_left-=bytes_written;
            printf("Sent %d of %d\n",(fsize-bytes_left),fsize );
        }
        printf("%s File Sent Successfully \n", filename);
    }
    bzero(file_buffer,BUFFER_SIZE);
    fclose(f);


}

//process message and sent response
void onMessage(char *buffers){
    update_file_list();
    printf("Message Recv : %s\n",buffer);
    int choice=  atoi(strtok(buffer, ","));
    char *msg1= strtok(NULL, ",");      //Assuming there are only 3 arg passed...
    char *msg2=strtok(NULL, ",");
    snprintf(auth_user, sizeof(auth_user),"%s",msg1);
    // printf("///%s\n",auth_user );
    snprintf(response_message, sizeof(response_message), "--No message--");
    switch (choice) {
        case 1: if(usernameAvailable(msg1))
                    snprintf(send_message, sizeof(response_message), "true");
                else
                    snprintf(send_message, sizeof(response_message), "false");
                sendMessage();
                break;
        case 2: if (!createUser(msg1,msg2))
                    snprintf(send_message, sizeof(response_message), "true");
                 else
                    snprintf(send_message, sizeof(response_message), "false");
                 sendMessage();
                 break;
        case 3: if (authenticateUser(msg1,msg2))
                    snprintf(send_message, sizeof(response_message), "true");
                 else
                    snprintf(send_message, sizeof(response_message), "false");
                 sendMessage();
                 break;
        case 4: fileRecieve(); break;
        case 5: fileSend();break;
        case 0: closeConnection();exit(0);
        default : printf("Admin, we have a issue..! %d %s %s\n",choice,msg1,msg2);

    }

    printf("%d %s %s\n",choice,msg1,msg2 );
}


void start_server(){
        int n;
        while(true){
            bzero(buffer,256);
            n = read(newsockfd,buffer,255);
            if (n < 0) error("ERROR reading from socket");
            onMessage(buffer);
            // n = write(newsockfd,response_message,strlen(response_message));
            // printf("Message Send: %s\n",response_message );
            if (n < 0) error("ERROR writing to socket");
        }
}



int main(int argc, char *argv[]){
     establishConenction();
     start_server();
     closeConnection();
     return 0;
}
