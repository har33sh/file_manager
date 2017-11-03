#include<iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include<dirent.h>
#include <string>
#include <sys/signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <sys/resource.h>
#include<signal.h>
#include <sys/types.h>
#include <sys/syscall.h>

using namespace std;

//config
bool  debug=false,show_msg=false;

#define BUFFER_SIZE 256
char file_dir[]="/home/ghost/Downloads/Data";
char file_list[]="/home/ghost/file_list/";
int PORT,display_files=10;



struct sockaddr_in serv_addr, cli_addr;
socklen_t clilen;
struct args {
    int socket;
};


void *serve_request(void*);

//\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\
////////////////////////////////   Network Layer  \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\
//\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\


void error(const char *msg,int socket){
    cerr<<"Error! Process ID : " <<getpid()<<"  "<<msg<<"   Socket:"<<socket<<endl;
    // Need to close the sockets and the threads that cause the error
    close(socket);
    pthread_exit(0);
}

//socket, bind, listen
void start_file_server(){
    printf("Starting File server ....\n");
    int sockfd;
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
      error("ERROR opening socket",sockfd);
    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(PORT);
    if (bind(sockfd, (struct sockaddr *) &serv_addr,
    sizeof(serv_addr)) < 0)
      error("ERROR on binding",sockfd);
    listen(sockfd,5);
    clilen = sizeof(cli_addr);
    pid_t pid;
    while(true){
        int con_sock = accept(sockfd,(struct sockaddr *) &cli_addr,&clilen);
        cout<<"Proxy Server connected || Socket :"<<con_sock <<endl;
        struct args *socket_fd;
        socket_fd=(args*)malloc(sizeof(struct args));
        socket_fd->socket=con_sock;
        pthread_t file_server_thread;
        if (pthread_create(&file_server_thread,NULL,serve_request,socket_fd) !=0 ){
          free(socket_fd);
          error("Thread Creation Failed",con_sock);
        }
    }
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
      error("ERROR reading from socket :: receiveMessage",socket_id);
  }
  return bytes_read;
}

//\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\
//////////////////////////////// Data Access Layer///////////////////////////////////////////////
//\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\


void fileRecieve(int socket_id){
    if (debug)  cout<<"======== File Receiving ========="<<endl;
    char file_buffer[BUFFER_SIZE],message[BUFFER_SIZE],file_location[100];
    FILE *fp;
    int bytes_left,bytes_read,bytes_written;
    //Init
    snprintf(message,sizeof(message),"%s","Started fileReceive()");
    send_message(socket_id,message,strlen(message));
    recv_message(socket_id,message);
    int filesize=  atoi(strtok(message, ","));
    char *filename= strtok(NULL, ",");
    if (debug) printf("\tReceiving File: %s FileSize: %d\n",filename,filesize);
    snprintf(file_location,sizeof(file_location),"%s/%s",file_dir,filename); //can add auth_user here
    if (debug) printf("Writing File to %s\n",file_location );
    fp = fopen(file_location, "wb");
    snprintf(message, sizeof(message), "%s","Ready to recive");
    send_message(socket_id,message,strlen(message));
    //Recv
    bzero(file_buffer,BUFFER_SIZE);
    bytes_left=filesize;
    while (bytes_left>0){ //Try to optimize
        bytes_read = read(socket_id,file_buffer,min(BUFFER_SIZE,bytes_left)) ;
        if (bytes_read <=0){
            error("ERROR reading from socket :: File Receiving",socket_id);
        }
        bytes_written=fwrite(file_buffer, sizeof(char), bytes_read, fp);
        bytes_left-=bytes_written;
        if (debug)printf("-----> Receiving %d of %d\n",(filesize-bytes_left),filesize );
        snprintf(message, sizeof(message),"%d %d %d",(filesize-bytes_left),filesize,bytes_left);
        bytes_written = send_message(socket_id,message,strlen(message));
        if (debug)  printf("<-----%d %s\n",bytes_written,message);
        }
    //Term
    fclose(fp);
    if (debug) printf("File Received: %s File Size: %d  \n",filename,filesize );
    if (debug) printf("%d ========End of File Receiving =========\n",socket_id);
}




void fileSend(int socket_id){
  if (debug) printf("%d ======== File Sending =========\n",getpid());

  //Update List of Files available
  int available_files=0;
  char file_buffer[BUFFER_SIZE],file_names[12][100],message[BUFFER_SIZE],file_list_name[BUFFER_SIZE];
  struct dirent *dir;
  DIR *d;
  d = opendir(file_dir);
  FILE *f;
  snprintf(file_list_name,sizeof(file_list_name),"%s%d.txt",file_list,socket_id); //Every user has unique socket
  f=fopen(file_list_name,"w");
  int i=1;
  if (debug) printf("File List %s\n",file_list_name );
  while ((dir = readdir(d)) != NULL){
    snprintf(file_names[i],sizeof(file_names[i]),"%s",dir->d_name);
    fprintf(f, "%d : %s\n",i,file_names[i] );
    if (debug) printf("%d : %s\n",i,file_names[i] );
    i++;
    if (i>10) break ; // quick fix, a page can show only 10 entries
  }
  available_files=i;
  closedir(d);
  fclose(f);
  if (debug) printf("Updated file list\n");
  //End of file upload

  //Send File List
  f= fopen(file_list_name, "rb");
  fseek(f, 0, SEEK_END);
  int filesize = ftell(f);
  rewind(f);
  snprintf(message, sizeof(message), "%d,%s", filesize,"file_list");
  send_message(socket_id,message,strlen(message));
  recv_message(socket_id,message);
  int  bytes_left=filesize,bytes_read,bytes_written;
  if (debug) printf("Sending the file list ...... Size %d\n",filesize);
  bzero(file_buffer,BUFFER_SIZE);
  while (bytes_left>0){
      bytes_read = fread(file_buffer,sizeof(char), BUFFER_SIZE, f);
      bytes_written = send_message(socket_id, file_buffer, bytes_read);
      bytes_left-=bytes_written;
  }
  if (debug) printf("%d File list Successfully\n",getpid());
  bzero(file_buffer,BUFFER_SIZE);
  fclose(f);
  //End of File List Sending

  recv_message(socket_id,message); //get the file choice from client
  int choice=  atoi(strtok(message, ","));
  if (debug) printf("input %d\n",choice );
  char filename[100];
  if(choice>available_files)
    error("Invalid request for file",socket_id);
  snprintf(filename,sizeof(filename),"%s/%s",file_dir,file_names[choice]);
  bzero(file_buffer,BUFFER_SIZE);
  f= fopen(filename, "rb");
  if (f == NULL) error("File not found!",socket_id);
  else{
      //Check the file size
      fseek(f, 0, SEEK_END);
      int fsize = ftell(f);
      rewind(f);
      snprintf(message, sizeof(message), "%d,%s", fsize,file_names[choice]);
      send_message(socket_id,message,strlen(message));
      bytes_left=fsize;
      if (debug) printf("Requesting the file FileSize : %d  FileName : %s \n",fsize,filename);
      recv_message(socket_id,message);

      //file sending
      while (bytes_left>0){
          bytes_read = fread(file_buffer,sizeof(char), min(BUFFER_SIZE,bytes_left), f);
          if (bytes_read <=0){
              error("ERROR reading from socket :: File Sending",socket_id);
          }
          bytes_written = send_message(socket_id, file_buffer, bytes_read);
          bzero(message,BUFFER_SIZE);
          int bytes_read=read(socket_id,message,BUFFER_SIZE);
          if (debug) printf("----->%d %s\n",bytes_read,message);
          if (bytes_read <=0){
              error("ERROR reading from socket :: receiveMessage",socket_id);
          }
          bytes_left-=bytes_written;
          if (debug) printf("Sent %d of %d\n",(fsize-bytes_left),fsize );
      }
      if (debug) printf("%s File Sent Successfully \n", filename);
  }

  bzero(file_buffer,BUFFER_SIZE);
  fclose(f);
  if (debug) printf("%d ======== End of File Sending =========\n",getpid());
}



void  *serve_request(void* x){
  if (debug) printf("%d ---King's Landing---\n", getpid());
  struct args *socket_fd=(struct args*)x;
  int socket_id=socket_fd->socket;
  while(true){
      char buffer[BUFFER_SIZE];
      recv_message(socket_id,buffer);
      if (debug) printf("Message Recv : %s\n",buffer);
      int choice=  atoi(strtok(buffer, ","));
      char *msg1= strtok(NULL, ",");      //Message format fixed with 3 arguments
      char *msg2=strtok(NULL, ",");
      switch (choice) {
        case 4: fileRecieve(socket_id); break;
        case 5: fileSend(socket_id);break;
        case 0: close(socket_id);
        default : error("Invalid request placed -- Cannot Serve Request",socket_id);
      }
    }
}



int main(int argc, char *argv[]){
      if(argc<2){
        fprintf(stderr,"usage %s listen_port\n", argv[0] );
        exit(0);
      }
     PORT=atoi(argv[1]);
     start_file_server();
     return 0;
}
