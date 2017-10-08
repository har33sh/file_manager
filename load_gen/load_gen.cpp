
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
#include <sys/time.h>

using namespace std;

//Config
bool debug =false;
char *filename = (char *)"/a.txt";
char HOST[50];
int PORT;
#define BUFFER_SIZE 256
int type_load;
struct timeval stop, start;
//\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\
////////////////////////////////   Network Layer  \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\
//\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\

void error(const char *msg){
    cerr <<"  "<< msg << endl;
    printf("%s\n",msg );
    pthread_exit(0);
}

//Establish connection to server
int establishConenction(){
    struct sockaddr_in serv_addr;
    struct hostent *server;
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0){
      char temp[100];
      snprintf(temp,sizeof(temp),"ERROR opening socket :: sockfd returned %d",sockfd );
      error(temp);
    }
    server = gethostbyname(HOST);
    if (server == NULL) {
      char temp[100];
      snprintf(temp,sizeof(temp),"ERROR,Host not found:: server returned %d",sockfd );
      error(temp);
    }
    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr,
    (char *)&serv_addr.sin_addr.s_addr,
    server->h_length);
    serv_addr.sin_port = htons(PORT);
    int connect_ret=connect(sockfd,(struct sockaddr *) &serv_addr,sizeof(serv_addr));
    if (connect_ret < 0){
      char temp[100];
      snprintf(temp,sizeof(temp),"ERROR,Connecting:: connect returned %d",connect_ret );
      error(temp);
    }
    return sockfd;
}


void sendMessage(int sockfd,char *send_message){
        int bytes_written = write(sockfd,send_message,strlen(send_message));
        if(debug)
             printf("<-----%d %s\n",bytes_written,send_message);
        if (bytes_written < 0){
          char temp[100];
          snprintf(temp,sizeof(temp),"ERROR writing to socket: socket write returned %d",bytes_written );
          error(temp);
        }

}

char* receiveMessage(int sockfd, char *response_message){
    bzero(response_message,BUFFER_SIZE);
    int bytes_read=read(sockfd,response_message,BUFFER_SIZE);
    if (debug)
         printf("----->%d %s\n",bytes_read,response_message);
    if (bytes_read < 0){
      char temp[100];
      snprintf(temp,sizeof(temp),"ERROR reading to socket: socket read returned = %d",bytes_read );
      error(temp);
    }
    return response_message;
}


void closeConnection(int sockfd){
    char send_message[BUFFER_SIZE];
    snprintf(send_message, sizeof(send_message), "%s,%s", "0","Byee..");
    int n = write(sockfd,send_message,strlen(send_message));
    close(sockfd);
    printf("Closed conection %d\n",sockfd);
}




//\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\
//////////////////////////////// Data Access Layer///////////////////////////////////////////////
//\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\

bool verify_ack(int left,char response_message[BUFFER_SIZE]){
  int success,total,remaining;
  sscanf(response_message,"%d %d %d", &success,&total,&remaining);
  if(remaining==left)
    return true;

  char temp[100];
  snprintf(temp,sizeof(temp),"Ack verify failed %d %d\n",left,remaining  );
  error(temp);
  return false;
}


//upload file
int upload(int sockfd){
    char username[20];
    char response_message[BUFFER_SIZE],send_message[BUFFER_SIZE],file_buffer[BUFFER_SIZE];
    snprintf(send_message, sizeof(send_message),"%d,%s", 4,"username");
    gettimeofday(&start, NULL);
    sendMessage(sockfd,send_message);
    receiveMessage(sockfd,response_message);
    gettimeofday(&stop, NULL);
    char log_file_name[100];
    snprintf(log_file_name,sizeof(log_file_name),"response_logs/%d.log",sockfd);
    FILE *log_response_time= fopen(log_file_name,"a");
    if ((stop.tv_usec - start.tv_usec )>0)
      fprintf(log_response_time,"%ld\n", stop.tv_usec - start.tv_usec);
    fclose(log_response_time);
    int fsize,bytes_read,bytes_written,bytes_left;
    bzero(file_buffer,BUFFER_SIZE);
    FILE *f= fopen(filename, "rb");
    if (f == NULL   ){
      char temp[100];
      snprintf(temp,sizeof(temp),"%s File not found!\n",filename);
      error(temp);

    }
    else{
        fseek(f, 0, SEEK_END);
        fsize = ftell(f);
        rewind(f);
        snprintf(send_message, sizeof(send_message), "%d,%d_ignore", fsize,sockfd);
        sendMessage(sockfd,send_message);
        bytes_left=fsize;
        if (debug) printf("FileSize : %d  FileName : %s save as : %d_ignore ...... \n",fsize,filename,sockfd);
        receiveMessage(sockfd,response_message);
        while (bytes_left>0){
            bytes_read = fread(file_buffer,sizeof(char), BUFFER_SIZE, f);
            bytes_written = write(sockfd, file_buffer, bytes_read);
            if (bytes_written < 0){
              char temp[100];
              snprintf(temp,sizeof(temp),"ERROR writing to socket in upload: socket write returned = %d",bytes_written );
              error(temp);
            }
            bytes_left-=bytes_written;
            if (debug)
                printf("%d<----Uploaded %d of %d\n",sockfd,(fsize-bytes_left),fsize );
            receiveMessage(sockfd,response_message);
            if(!verify_ack(bytes_left,response_message)){
                  printf("Bad Ack Received %d %d\n",bytes_read,bytes_written );
                  break;
            }
        }
    }
    bzero(file_buffer,BUFFER_SIZE);
    fclose(f);
    snprintf(log_file_name,sizeof(log_file_name),"throughput/%d.log",sockfd);
    log_response_time= fopen(log_file_name,"a");
    fprintf(log_response_time,"1\n");
    fclose(log_response_time);
    if (debug) printf("%d | %s File Successfully uploaded\n",sockfd, filename);
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
    gettimeofday(&stop, NULL);
    char log_file_name[100];
    snprintf(log_file_name,sizeof(log_file_name),"response_logs/%d.log",sockfd);
    FILE *log_response_time= fopen(log_file_name,"a");
    if ((stop.tv_usec - start.tv_usec )>0)
      fprintf(log_response_time,"%ld\n", stop.tv_usec - start.tv_usec);
    fclose(log_response_time);
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
        if (bytes_read < 0){
          char temp[100];
          snprintf(temp,sizeof(temp),"ERROR reading to socket in load file list: socket read returned = %d",bytes_read );
          error(temp);
        }
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
    if(debug) printf("\n##### File List ####\n");
    while ((c=fgetc(fp))!=EOF)
      if(debug) printf("%c",c );
    if(debug) printf("\n####################\n");
    fclose(fp);
}

//needs to be checked if it works
void download(int sockfd){
    char username[20];
    char response_message[BUFFER_SIZE],send_message[BUFFER_SIZE],file_buffer[BUFFER_SIZE];
    snprintf(send_message, sizeof(send_message),"%d,%s", 5,"username");
    gettimeofday(&start, NULL);
    sendMessage(sockfd,send_message);
    load_file_list(sockfd);
    int choice=3;
    char save_file[100];
    snprintf(save_file, sizeof(save_file), "temp/%d_ignore",sockfd);
    if (debug) printf("Downloading... %s\n",save_file);
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
        if (bytes_read < 0){
          char temp[100];
          snprintf(temp,sizeof(temp),"ERROR reading to socket in download: socket read returned = %d",bytes_read );
          error(temp);
        }
        bytes_written=fwrite(file_buffer, sizeof(char), bytes_read, fp);
        snprintf(send_message, sizeof(send_message),"%d %d %d",(filesize-bytes_left),filesize,bytes_left);
        sendMessage(sockfd,send_message); //Ack Message
        bytes_left-=bytes_written;
        if (debug)
            printf("%d Receiving %d of %d\n",bytes_written,(filesize-bytes_left),filesize );
    }
    bzero(file_buffer,BUFFER_SIZE);
    fclose(fp);

    char log_file_name[100];
    snprintf(log_file_name,sizeof(log_file_name),"throughput/%d.log",sockfd);
    FILE *log_response_time= fopen(log_file_name,"a");
    fprintf(log_response_time,"1\n");
    fclose(log_response_time);

    printf("%d | %s file Downloaded,  Filesize: %d\n",sockfd , save_file,filesize );
}

//\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\
//////////////////////////////// Presentation Layer///////////////////////////////////////////////
//\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\

void reg(int sockfd){ //add random and confiure
    char username[20];
    char response_message[BUFFER_SIZE],send_message[BUFFER_SIZE];
    bool availablity=false;
    char password[20];
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
    receiveMessage(sockfd,response_message);
    if(response_message[0]=='t')
      auth=true;
    while(!auth){
        error("Bad username or password \n add user abc to DB ");
        }
    if (debug) printf("### user %s Successfully Authenticated  ###\n","abc" );
    return auth;
}

void logout(int sockfd){
  char send_message[BUFFER_SIZE];
  snprintf(send_message, sizeof(send_message), "%s", "0,Client logging out");
  sendMessage(sockfd,send_message);
  closeConnection(sockfd);exit(0);
}


void file_menu(int sockfd){
    long int iteration =1;
    while (true) {
        if (debug) printf("%ld  |  %d \n",iteration++ ,sockfd );
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


void *start_threads(void *threadid) {
   long tid;
   tid = (long)threadid;
   cout << "Starting Thread ID, " << tid<< endl;
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
    if (debug) cout << "Creating thread, " << i << endl;
    rc = pthread_create(&threads[i], NULL, start_threads, (void *)i);

    if (rc) {
       cerr << "Error:unable to create thread," << rc << endl;
       exit(-1);
    }
  }
  pthread_exit(NULL);
}
