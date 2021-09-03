#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <iostream>
#include <cstdlib>
#include <cstdio>

#include "shell1.h"

using namespace std;

int main(int argc,char *argv[])
{
    signal(SIGCHLD,SIG_IGN);
    int	 sockfd,newsockfd,port;
    struct sockaddr_in	cli_addr , serv_addr;
    socklen_t	clilen = sizeof (cli_addr);
    
    bzero(&serv_addr, sizeof(serv_addr));
    pid_t childpid;
   
    if(argc!=2)
        return -1;
    else
        port = atoi(argv[1]) ;
   
    if ( (sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
		cout << "server: can't open stream socket" << endl;
    
    bzero(&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET ;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);  
    serv_addr.sin_port = htons(port) ;

    if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
		cout << "server: can't bind local address"  ;
	listen(sockfd, 5);
    
    while(1) {
            clilen = sizeof(cli_addr);
            newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
            if (newsockfd < 0)  cout << "server: accept error" ;
            if ( (childpid = fork()) < 0)       cout << "server: fork error" ;
            else if (childpid == 0) {                	
                dup2(newsockfd, 0);
                dup2(newsockfd, 1);
                dup2(newsockfd, 2);
                close(sockfd) ;
                close(newsockfd);
                npshell();
                
                exit(0);
            }              
            close(newsockfd);  
        }
}