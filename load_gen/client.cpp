
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
#include <iostream>
#include <cstdlib>
#include <pthread.h>
#include <time.h>
#include <stdlib.h>

using namespace std;

//Config
bool debug =false;
char *filename = (char *)"/a.txt";
char HOST[50];
int PORT;
#define BUFFER_SIZE 256
int  portno, n;
int type_load;
//\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\
////////////////////////////////   Network Layer  \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\
//\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\

void error(const char *msg){
    perror(msg);
    exit(0);
}

//Establish connection to server
int establishConenction(){
    struct sockaddr_in serv_addr;
    struct hostent *server;
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

char* receiveMessage(int sockfd, char *response_message){
    bzero(response_message,BUFFER_SIZE);
    int bytes_read=read(sockfd,response_message,BUFFER_SIZE);
    if (debug)
         printf("----->%d %s\n",bytes_read,response_message);
    if (bytes_read < 0)
        error("ERROR reading from socket");
    return response_message;
}


void closeConnection(int sockfd){
    char send_message[BUFFER_SIZE];
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

bool verify_ack(int left,char response_message[BUFFER_SIZE]){
  char *_=  strtok(response_message, ",");
  int  ack_sent= atoi(strtok(NULL, ","));
  //printf("%d %d\n",left,ack_sent );
  if(ack_sent==left)
    return true;
  return false;
}

//upload file
int upload(int sockfd){
    char username[20];
    char response_message[BUFFER_SIZE],send_message[BUFFER_SIZE],file_buffer[BUFFER_SIZE];
    snprintf(send_message, sizeof(send_message),"%d,%s", 4,username);
    sendMessage(sockfd,send_message);
    receiveMessage(sockfd,response_message);
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
        snprintf(send_message, sizeof(send_message), "%d,%d_ignore", fsize,sockfd);
        sendMessage(sockfd,send_message);
        bytes_left=fsize;
        //printf("FileSize : %d  FileName : %s \nUploading the file...... \n",fsize,filename);
        receiveMessage(sockfd,response_message);
        while (bytes_left>0){
            bytes_read = fread(file_buffer,sizeof(char), BUFFER_SIZE, f);
            bytes_written = write(sockfd, file_buffer, bytes_read);
            bytes_left-=bytes_written;
            // printf("%s\n",file_buffer );
            if (debug)
                printf("<----Uploaded %d of %d\n",(fsize-bytes_left),fsize );
            receiveMessage(sockfd,response_message);
            if(!verify_ack(bytes_left,response_message)){
                  printf("Bad Ack Received\n" );
                  break;
            }
        }
        //printf("%s File Successfully uploaded\n", filename);
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

void load_file_list(int sockfd){
    char response_message[BUFFER_SIZE],send_message[BUFFER_SIZE],file_buffer[BUFFER_SIZE];
    receiveMessage(sockfd,response_message);
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

//needs to be checked if it works
void download(int sockfd){
    char username[20];
    char response_message[BUFFER_SIZE],send_message[BUFFER_SIZE],file_buffer[BUFFER_SIZE];
    snprintf(send_message, sizeof(send_message),"%d,%s", 5,username);
    sendMessage(sockfd,send_message);
    load_file_list(sockfd);
    int choice=2;
    char save_file[100];
    snprintf(save_file, sizeof(save_file), "%d_ignore",sockfd);
    //printf("Downloading... %s\n",save_file);
    snprintf(send_message, sizeof(send_message), "%d,",choice);
    sendMessage(sockfd,send_message); //sending the choice
    if (debug)
        printf("waiting for file details\n" );
    receiveMessage(sockfd,response_message); //gets the file details
    int filesize=  atoi(strtok(response_message, ","));
    int bytes_left,bytes_read,bytes_written;
    FILE *fp = fopen(save_file, "wb");
    bytes_left=filesize;
    bzero(file_buffer,BUFFER_SIZE);
    snprintf(send_message, sizeof(send_message), "%s,","Ready for download");
    sendMessage(sockfd,send_message); //starting Download
    while (bytes_left>0){
        bytes_read = read(sockfd,file_buffer,min(BUFFER_SIZE,bytes_left)) ;
        if(bytes_read==-1){
          printf("%s\n", "Socket cant read");
        }
        bytes_written=fwrite(file_buffer, sizeof(char), bytes_read, fp);
        snprintf(send_message, sizeof(send_message),"Ack  %d of %d,%d,1",(filesize-bytes_left),filesize,bytes_left);
        sendMessage(sockfd,send_message); //Ack Message
        bytes_left-=bytes_written;
        if (debug)
            printf("%d Receiving %d of %d\n",bytes_written,(filesize-bytes_left),filesize );
    }
    bzero(file_buffer,BUFFER_SIZE);
    fclose(fp);
    // receiveMessage(sockfd,response_message); //Server sends a completion flag from the whil true loop
    printf("Download File: %s filesize: %d\n",save_file,filesize );
}

//\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\
//////////////////////////////// Presentation Layer///////////////////////////////////////////////
//\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\

void reg(int sockfd){ //add random and confiure
    char username[20];
    char response_message[BUFFER_SIZE],send_message[BUFFER_SIZE];
    bool availablity=false;
    char password[20];
    //printf("Enter username : ");
    scanf("%s",username );
    snprintf(send_message, sizeof(send_message), "%s,%s", "1",username);
    sendMessage(sockfd,send_message);
    receiveMessage(sockfd,response_message);
    if(response_message[0]=='t')
      availablity=true;
    while(!availablity){
        printf("%s already taken, try a differnt one...\nEnter username :",username );
        scanf("%s",username );
        snprintf(send_message, sizeof(send_message), "%s,%s", "1",username);
        sendMessage(sockfd,send_message);
        receiveMessage(sockfd,response_message);
        if(response_message[0]=='t')
          availablity=true;
    }
    printf("Enter password :");
    scanf("%s",password );
    printf("%s registered Successfully\n",username );
    snprintf(send_message, sizeof(send_message), "%s,%s,%s", "2",username,password);
    sendMessage(sockfd,send_message);
}

bool login(int sockfd){
    char username[20];
    char response_message[BUFFER_SIZE],send_message[BUFFER_SIZE];
    bool auth=false;
    char yes_or_no[2];
    char password[20];
    snprintf(send_message, sizeof(send_message), "%s,%s,%s", "3","abc","abc");
    sendMessage(sockfd,send_message);
    printf("ffff\n" );
    receiveMessage(sockfd,response_message);
    printf("see %s",response_message );
    if(response_message[0]=='t')
      auth=true;
    while(!auth){
        printf("Bad username or password \n add user abc to DB ");
        exit(0);
        }
    printf("### user %s Successfully Authenticated  ###\n",username );
    return auth;
}

void logout(int sockfd){
  char send_message[BUFFER_SIZE];
  snprintf(send_message, sizeof(send_message), "%s", "0,Client logging out");
  sendMessage(sockfd,send_message);
  closeConnection(sockfd);exit(0);
}


//later modify with private file storage --> If needed
void file_menu(int sockfd){
    while (true) {
        switch (type_load) {
            case 1: upload(sockfd); break;
            case 2: download(sockfd); break;
            case 3: upload(sockfd),download(sockfd);break;
            default: printf("Invalid Option selected..!! \n");
        }
    }
}

void menu(int sockfd){
    int choice;
    printf("//////////////////////////////////////////////////////////////////////////////\n");
    printf("///////////////////////////        FILE SERVER         ///////////////////////\n");
    printf("//////////////////////////////////////////////////////////////////////////////\n");
    while (true) {
        choice=2;
        switch (choice) {
            case 1: reg(sockfd); break;
            case 2: if(login(sockfd)) file_menu(sockfd); break;
            case 3: closeConnection(sockfd);exit(0);
            default: printf("Invalid Option selected..!! \n");
        }
    }
}



//////////////////////////////// MAIN



#define NUM_THREADS 5

void *PrintHello(void *threadid) {
   long tid;
   tid = (long)threadid;
   int random_number = rand();
   cout << "Starting Thread ID, " << tid <<" "<<random_number<< endl;
   int sockfd= establishConenction();
   menu(sockfd);
   closeConnection(sockfd);
   pthread_exit(NULL);
}

int main (int argc, char *argv[]) {
  if(argc<5){
    fprintf(stderr,"usage %s <Server_IP> <Server_Port> <Threads> <1:Upload 2:Download 3:Upload&Download>\n", argv[0] );
    exit(0);
  }
  snprintf(HOST,sizeof(HOST),"%s",argv[1]);
  PORT=atoi(argv[2]);
  int no_of_threads=atoi(argv[3]);
  type_load=atoi(argv[4]);
  if(type_load<1 || type_load >3){
    fprintf(stderr,"usage %s <Server_IP> <Server_Port> <Threads> <1:Upload 2:Download 3:Upload&Download>\n", argv[0] );
    exit(0);
  }
  pthread_t threads[no_of_threads];
  int rc;
  int i;
  srand(time(NULL));
  for( i = 0; i < no_of_threads; i++ ) {
    cout << "Creating thread, " << i << endl;
    rc = pthread_create(&threads[i], NULL, PrintHello, (void *)i);

    if (rc) {
       cout << "Error:unable to create thread," << rc << endl;
       exit(-1);
    }
  }
  pthread_exit(NULL);
}
