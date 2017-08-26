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
#include <wordexp.h>
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
#define BUFFER_SIZE 256
using namespace std;
vector< pair <string, string> > list;
time_t rawtime;
tm* timeinfo;


void error(const char *msg)
    perror(msg);


int sendSearchRequest(int sockfd, string searchText ,struct sockaddr_in serv_addr){
   sockfd = socket(AF_INET, SOCK_STREAM, 0);
   if (connect(sockfd,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0){
      error("Server Unreachable")  ;
      close(sockfd);
      return 0;
      }

const char * buffer=searchText.c_str();
char requestString[strlen(buffer)+10];
strcpy(requestString,"search#@#");
strcat(requestString,buffer);
int n= write(sockfd,requestString,strlen(requestString));
if (n < 0)
{
error("ERROR writing to socket");
close(sockfd);
return 0;
}
char buff[256];
bzero(buff,256);
n = read(sockfd,buff,255);
if (n < 0)
{
error("ERROR reading from socket");
close(sockfd);
return 0;
}
//printf("%s\n",buff);
if(strncmp(buff,"!!",2)==0)
{
perror(&buff[2]);//file not found
close(sockfd);
return 0;
}
else
parseData(buff);
close(sockfd);
return 1;
}

 void expandSourcePath(char * source,char * destination){
   wordexp_t exp_result;
   wordexp(source, &exp_result, 0);
   strcpy(destination,exp_result.we_wordv[0]);
   wordfree(&exp_result);
   }

// logs message in fomat : timestamp + message +ip
//hard coding client.log
void writeToLogFile(const char * message,char * ip){
  char buffer[200];
  time(&rawtime);
  timeinfo=localtime(&rawtime);
  std::strftime(buffer,80,"%d-%m-%Y %H-%M-%S: ",timeinfo);
  strcat(buffer,message);
  strcat(buffer,ip);
  strcat(buffer,"\n");
  ofstream logFile;
  logFile.open("client.log",ios::out | ios::app);
  if(!logFile){
    logFile.close();
    return ;
  }
  logFile<<buffer;
  logFile.close();
}



void startAndRunDownloadServer(){
   int listenfd = 0;
   int connfd = 0;
   struct sockaddr_in serv_addr,cli_addr;
   socklen_t clilen;
   listenfd = socket(AF_INET, SOCK_STREAM, 0);
   //printf("Socket retrieve success\n");
   memset(&serv_addr, '0', sizeof(serv_addr));
   serv_addr.sin_family = AF_INET;
   serv_addr.sin_addr.s_addr = INADDR_ANY;
   serv_addr.sin_port = htons(8996);

   if(bind(listenfd, (struct sockaddr*)&serv_addr,sizeof(serv_addr))<0)
      error(" ds Error in binding");

   if(listen(listenfd, 10) == -1){
      perror("Failed to listen\n");
      return ;
      }

   char buffer[256];
   while(true){
      clilen = sizeof (cli_addr);
      connfd = accept(listenfd, (struct sockaddr * )&cli_addr,&clilen);
      if (connfd < 0)
         error ("ERROR on accept");
      int ppid=fork();
      if(ppid==0){
         //cout<<"chile works with pid  "<<ppid<<"conn fd"<<endl;
         bzero (buffer, 256);
         int n = read (connfd, buffer, 255);
         if (n < 0)
            error ("ERROR reading from socket");
         char clntIP[INET_ADDRSTRLEN];
         inet_ntop(AF_INET, (struct inaddr *)&cli_addr.sin_addr,clntIP,sizeof(clntIP));
         writeToLogFile("Download request from ",clntIP);
         int inFile;
         int n_char = 0;
         char outputBuffer[BUFFER_SIZE];
         if(buffer[0]=='~'){
            char temp[BUFFER_SIZE];
            strcpy(temp,buffer);
            expandSourcePath(temp,buffer);
            }
         inFile = open (buffer, O_RDONLY);
         if (inFile == -1){
            perror("error");
            std::ostringstream s;
            s << errno;
            string message(s.str());
            strcpy(outputBuffer,"Error");
            strcat(outputBuffer,message.c_str());
            write(connfd,outputBuffer,BUFFER_SIZE);
            }
         else{
            while ((n_char = read (inFile, outputBuffer, BUFFER_SIZE)) != 0){
               write(connfd, outputBuffer, n_char);
               }
            }//else
         close (inFile);
         close(connfd);
         writeToLogFile("File sent to ",clntIP);
         }//if ppid
      else
         close(connfd);
      }//while infinite loop
}



int main(int argc, char *argv[]){
   int pid=fork();
   if(pid==0)
      startAndRunDownloadServer();
   else{
      // cout<<"parent\n";
      int sockfd, portno;
      struct sockaddr_in serv_addr;
      struct hostent *server;
      if (argc < 3) {
         fprintf(stderr,"usage %s hostname port\n", argv[0]);
         exit(0);
         }
   portno = atoi(argv[2]);
   //sockfd = socket(AF_INET, SOCK_STREAM, 0);
   //if (sockfd < 0)
   //  error("ERROR opening socket");
   server = gethostbyname(argv[1]);
   if (server == NULL) {
      fprintf(stderr,"ERROR, no such host\n");
      exit(0);
      }
   bzero((char *) &serv_addr, sizeof(serv_addr));
   serv_addr.sin_family = AF_INET;
   bcopy((char *)server->h_addr,
   (char *)&serv_addr.sin_addr.s_addr,
   server->h_length);
   serv_addr.sin_port = htons(portno);
   while(true){
      cout<<endl<<"Select:"<<endl;
      cout<<"1. Search"<<endl;
      cout<<"2. Share"<<endl;
      cout<<"3. Exit"<<endl;
      char input;
      fflush(stdin);
      cin>>input;
      switch(input){
      case '1':{
            string searchText;
            cout<<"Type string to search : ";
            cin>>searchText;
            if(!sendSearchRequest(sockfd,searchText,serv_addr))
               break;
            cout<<"Select a mirror:"<<endl;
            int number=0;
            for(unsigned int i=0;i<list.size();i++)
               cout<<i+1<<"    "<<list[i].first<<"        "<<list[i].second<<endl;
            cout<<">> ";
            cin>>number;
            if(number>(int)list.size()){
               cout<<"Select valid Option"<<endl;
               break;
               }
            if(downloadFileFormServer(number,searchText))//download file from download server
               cout<<"Download Complete !"<<endl;
            else
               cout<<"unable to downlaod file"<<endl;
            break;
            }
      case '2':{
            string path;
            cout<<"Path :";
            cin>>path;
            sockfd = socket(AF_INET, SOCK_STREAM, 0);
            if (connect(sockfd,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0){
               error("Server Unreachable");
               close(sockfd);
               break;
               }
            else if(sendShareData(sockfd,path))
               cout<<"File Shared Successfully on repo!"<<endl;
            close(sockfd);
            break;
            }
      case '3':return 0;
      default : cout << "Invalid option ";
         }//switch
      } //while
   }//else
   return 0;
   }
