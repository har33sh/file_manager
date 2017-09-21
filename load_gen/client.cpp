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
#include <time.h>
#include <stdlib.h>
#include <iostream>
#include <cstdlib>
#include <pthread.h>
#include <time.h>
#include <stdlib.h>
using namespace std;

//Config
bool debug =false;
char HOST[50];
// #define HOST "10.129.23.200"
// #define PORT 4334
int PORT;
int global_choice;
#define BUFFER_SIZE 256
char *f_name = "/a.txt";
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
int establishConenction(){
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
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
    return sockfd;
}


void sendMessage(int sockfd,char *send_message){
        int bytes_written = write(sockfd,send_message,strlen(send_message));
        if(debug)
            printf("<-----%d %s\n",bytes_written,send_message);
        if (bytes_written < 0)
            error("ERROR writing to socket");

}

char* receiveMessage(int sockfd){
    char response_message[BUFFER_SIZE];
    bzero(response_message,BUFFER_SIZE);
    int bytes_read=read(sockfd,response_message,BUFFER_SIZE);
    if (debug)
        printf("----->%d %s\n",bytes_read,response_message);
    if (bytes_read < 0)
        error("ERROR reading from socket");
    return response_message;
}


void closeConnection(int sockfd,char *send_message){
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

bool verify_ack(int left,char* response_message){
  char *_=  strtok(response_message, ",");
  int  ack_sent= atoi(strtok(NULL, ","));
  printf("%d %d\n",left,ack_sent );
  if(ack_sent==left)
    return true;
  return false;
}

//upload file
int upload(int sockfd){
    char send_message[BUFFER_SIZE];
    snprintf(send_message, sizeof(send_message),"%d,%s", 4,username);
    sendMessage(sockfd,send_message);
    char *response_message= receiveMessage(); //ACK
    char filename[BUFFER_SIZE],save_as[BUFFER_SIZE];
    snprintf(filename, sizeof(filename), "%s", f_name);
    snprintf(save_as, sizeof(save_as), "12344"); //EDIT THIS
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
        sendMessage(sockfd,send_message);
        bytes_left=fsize;
        printf("FileSize : %d  FileName : %s \nUploading the file...... \n",fsize,filename);
        char* response_message= receiveMessage();
        while (bytes_left>0){
            bytes_read = fread(file_buffer,sizeof(char), BUFFER_SIZE, f);
            bytes_written = write(sockfd, file_buffer, bytes_read);
            bytes_left-=bytes_written;
            // printf("%s\n",file_buffer );
            if (debug)
                printf("<----Uploaded %d of %d\n",(fsize-bytes_left),fsize );
            char *response_message= receiveMessage();
            if(!verify_ack(bytes_left)){
                  printf("Bad Ack Received\n" );
                  break;
            }
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
    char response_message[BUFFER_SIZE],send_message[BUFFER_SIZE],file_buffer[BUFFER_SIZE];
    char *response_message=receiveMessage();
    int filesize=  atoi(strtok(response_message, ","));
    char *filename= strtok(NULL, ",");
    if(debug)
        printf("Receiving File List: %s FileSize: %d\n",filename,filesize);
    int bytes_left,bytes_read,bytes_written;
    snprintf(send_message, sizeof(send_message), "%s","Time to download");
    sendMessage(sockfd,send_message);
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

void download(){
    char response_message[BUFFER_SIZE],send_message[BUFFER_SIZE],file_buffer[BUFFER_SIZE];
    snprintf(send_message, sizeof(send_message),"%d,%s", 5,username);
    sendMessage();
    load_file_list();
    int choice=1;
    printf("Downloading... \n");
    snprintf(send_message, sizeof(send_message), "%d,",choice);
    sendMessage(); //sending the choice
    if (debug)
        printf("waiting for file details\n" );
    receiveMessage(); //gets the file details
    int filesize=  atoi(strtok(response_message, ","));
    int bytes_left,bytes_read,bytes_written;
    FILE *fp = fopen("xyz", "wb"); //EDIT THIS
    bytes_left=filesize;
    bzero(file_buffer,BUFFER_SIZE);
    snprintf(send_message, sizeof(send_message), "%s,","Ready for download");
    sendMessage(); //starting Download
    while (bytes_left>0){
        bytes_read = read(sockfd,file_buffer,min(BUFFER_SIZE,bytes_left)) ;
        if(bytes_read==-1){
          printf("%s\n", "Socket cant read");
        }
        bytes_written=fwrite(file_buffer, sizeof(char), bytes_read, fp);
        snprintf(send_message, sizeof(send_message),"Ack  %d of %d,%d,1",(filesize-bytes_left),filesize,bytes_left);
        sendMessage(); //Ack Message
        bytes_left-=bytes_written;
        if (debug)
            printf("%d Receiving %d of %d\n",bytes_written,(filesize-bytes_left),filesize );
    }
    bzero(file_buffer,BUFFER_SIZE);
    fclose(fp);
    // receiveMessage(); //Server sends a completion flag from the whil true loop
    printf("Download File filesize: %d\n",filesize );
}

//\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\
//////////////////////////////// Presentation Layer///////////////////////////////////////////////
//\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\

void reg(){
    char response_message[BUFFER_SIZE],send_message[BUFFER_SIZE],file_buffer[BUFFER_SIZE];
    bool availablity=false;
    char password[20];
    int random_number = rand();
    snprintf(username, sizeof(username), "%s%d", "random_user",random_number);
    printf("Username : %s",username );
    snprintf(send_message, sizeof(send_message), "%s,%s", "1",username);
    sendMessage();
    receiveMessage();
    if(response_message[0]=='t')
      availablity=true;
    while(!availablity){
        printf("%s already taken, trying a differnt one...\n",username );
        random_number = rand();
        snprintf(username, sizeof(username), "%s%d", "random_user",random_number);
        snprintf(send_message, sizeof(send_message), "%s,%s", "1",username);
        sendMessage();
        receiveMessage();
        if(response_message[0]=='t')
          availablity=true;
    }
    snprintf(password, sizeof(password), "%s%d", "random_password",random_number);
    printf("Enter password : %s",password );
    printf("%s registered Successfully\n",username );
    snprintf(send_message, sizeof(send_message), "%s,%s,%s", "2",username,password);
    sendMessage();
}

bool login(){
    char response_message[BUFFER_SIZE],send_message[BUFFER_SIZE],file_buffer[BUFFER_SIZE];
    bool auth=false;
    char yes_or_no[2];
    char password[20];
    snprintf(send_message, sizeof(send_message), "%s,%s,%s", "3","abc","abc");
    sendMessage();
    receiveMessage();
    if(response_message[0]=='t')
      auth=true;
    while(!auth){
        printf("Bad username or password \nTry again ? [Y/n] : ");
        exit(0);
    }
    printf("### user Successfully Authenticated  ###\n" );
    return auth;
}

void logout(){
  char response_message[BUFFER_SIZE],send_message[BUFFER_SIZE],file_buffer[BUFFER_SIZE];
  snprintf(send_message, sizeof(send_message), "%s", "0,Client logging out");
  sendMessage();
  closeConnection();exit(0);
}


//later modify with private file storage --> If needed
void file_menu(){
    int choice;
    while (true) {
        printf("1. Upload\n2. Download\n3. Mixed\n ");
        // scanf("%d",&choice);
        choice=global_choice;
        switch (choice) {
            case 1: upload(); break;
            case 2: download(); break;
            case 3: upload();download();return;
            default: printf("Invalid Option selected..!! \n");
        }
    }
}

void menu(){
    int choice;
    while (true) {
        printf("1. Register \n2. Login\n3. Exit\nEnter your Choice : ");
        choice=2; //Check here
        switch (choice) {
            case 1: reg(); break;
            case 2: if(login()) file_menu(); break;
            case 3: closeConnection();exit(0);
            default: printf("Invalid Option selected..!! \n");
        }
    }
}



//////////////////////////////// MAIN


void *PrintHello(void *threadid) {
   long tid;
   tid = (long)threadid;
   int random_number = rand();
   cout << "Starting Thread ID, " << tid <<" "<<random_number<< endl;
   establishConenction();
   menu();
   closeConnection();
   pthread_exit(NULL);
}


int main(int argc, char *argv[]){
  if(argc<5){
    fprintf(stderr,"usage %s (hostname) (server) (port) (no_of_threads) (0:Download/1:Upload/2:Mixed)\n", argv[0] );
    exit(0);
  }
    snprintf(HOST,sizeof(HOST),"%s",argv[1]);
    PORT=atoi(argv[2]);
    int NUM_THREADS=atoi(argv[3]);
    global_choice = atoi(argv[4]);

    pthread_t threads[NUM_THREADS];
    int rc;
    int i;
    srand(time(NULL));
    for( i = 0; i < NUM_THREADS; i++ ) {
       cout << "Creating thread, " << i << endl;
       rc = pthread_create(&threads[i], NULL, PrintHello, (void *)i);

       if (rc) {
          cout << "Error:unable to create thread," << rc << endl;
          exit(-1);
       }
    }
    pthread_exit(NULL);



    return 0;
}
