#include<stdio.h>
#include<string.h>
#include <iostream>
#include <string>
#include<stdlib.h>
#include<unistd.h>
#include<arpa/inet.h>
#include<sys/wait.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<sys/select.h>
# define MAXLINE 15000
#include<cstring>
//#include<algorithm>
#include<vector>
#include <iostream>
#include <cstdlib>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <unistd.h>
extern char ** environ;

using namespace std;
int parse(int sockfd,char* recv_buf,int msockfd) ;
void broadcast(int newsockfd,char* message) ;
void tell (int fd,char* message) ;
void who(int fd) ;
void rename(int fd,char* message);


int SERV_TCP_PORT;
struct client {
    int id;
	int fd;//user fd
	int index  ;
	char name[20];//user name
	char ip[30];//<ip>
    bool usage = false;
	int pipnum_arr[5000];
	int numpiper[5000][2] ;
	int total_n =0;
	int line_n  = -1 ;
  vector <string>  Path ;
  vector <string> variable_name;
}user[31];
int fd_to_index(int fd);
void splitStr2Vec(string s, vector<string>& buf);
int user_pipe[31][31][2]  ;
int dest;
int donumpipe ;
vector <char*> vecenvir ;

void npshell(int fd,char* recv_buf)
{
	
	int myid = fd_to_index(fd);
	clearenv();
	for(int i =0;i < vecenvir.size();i++){
		putenv(vecenvir[i]);
	}
	for(int i=0;i<user[myid].Path.size();i++){		
		setenv(user[myid].Path[i].c_str(),user[myid].variable_name[i].c_str(), 1);			
 	
	}
	char* val;
	int pid ;
	int i,j ;
	int dir_flag = 0;	
	string s ;
	s.assign(recv_buf) ;

	vector<string> buf;
	user[myid].total_n ++;
	int total = user[myid].total_n ;
	string numpi ;
		donumpipe = 0;
		splitStr2Vec(s, buf);
		int numpipe_exist = 0;
		int pipe_flag = 0 ;
		int err_flag = 0;
		int pipnum ;
		int redirect_flag = 0;
		int line ;
		int pip_to =-1;
		int send_id =-1;
		int clear_i = 0;
		int clear_o = 0;
		int self0=-1,self1=-1 ;
		for(int i=0;i<buf.size();i++)
		{
			char str[5000] ;
			if(buf[i] == "|") // pipe
				pipe_flag += 1 ;
			
			if( buf[i].find("<",0) != -1)
			{
				buf[i].erase(0,1);	
				send_id = atoi(buf[i].c_str()) ;	
				buf.erase(buf.begin()+i) ;
				if(send_id > 30){
					sprintf(str,"*** Error: user #%d does not exist yet. ***\n",send_id);
					tell(fd,str) ;	
					clear_i = 1 ;
					break ;
				}
				if(user[send_id].usage==false){
					sprintf(str,"*** Error: user #%d does not exist yet. ***\n",send_id);
					tell(fd,str) ;	
					clear_i = 1 ;
				}else if(user[send_id].usage==true && user_pipe[send_id][myid][0] == -10 && user_pipe[send_id][myid][1] == -10){
					sprintf(str,"*** Error: the pipe #%d->#%d does not exist yet. ***\n",user[send_id].id,user[myid].id);
					tell(fd,str);
					clear_i = 1 ;
				}
				else{					
					sprintf(str,"*** %s (#%d) just received from %s (#%d) by '%s' ***\n",user[myid].name,user[myid].id,user[send_id].name,user[send_id].id,recv_buf);
					//cerr << user_pipe[send_id][myid][1] << user_pipe[send_id][myid][0] << endl ;
					broadcast(fd,str) ;	
				}
				break ;
				
			}
		}
		for(int i=0;i<buf.size();i++)
		{
			if(buf[i] == "|") // pipe
				pipe_flag += 1 ;
			char str[5000] ;
			if (buf[i] == ">")
				redirect_flag = 1 ;
			else if(buf[i].find(">",0) != -1)
			{
				buf[i].erase(0,1);	
				pip_to = atoi(buf[i].c_str()) ;
				buf.erase(buf.begin()+i) ;
				if(pip_to>30){
					sprintf(str,"*** Error: user #%d does not exist yet. ***\n",pip_to);
					tell(fd,str);
					clear_o = 1 ;
					break ;
				}
				if(user[pip_to].usage!=true){
					sprintf(str,"*** Error: user #%d does not exist yet. ***\n",pip_to);
					tell(fd,str);
					clear_o = 1 ;
				}
				else if(user_pipe[myid][pip_to][0]==-10 ){
					pipe(user_pipe[myid][pip_to]); 
					sprintf(str,"*** %s (#%d) just piped '%s' to %s (#%d) ***\n",user[myid].name,user[myid].id,recv_buf,user[pip_to].name,user[pip_to].id);
					broadcast(fd,str);
				}
				else if (user_pipe[myid][pip_to][0]!=-10)
				{
					sprintf(str,"*** Error: the pipe #%d->#%d already exists. ***\n",user[myid].id,user[pip_to].id);
					tell(fd,str);
					clear_o = 1 ;
				}
				break;				
			}
			
		}
		for(int i=0;i<buf.size();i++)
		{
			if(buf[i] == "|") // pipe
				pipe_flag += 1 ;
			if((buf[i].find("|",0) != -1 && buf[i]!="|")|| buf[i].find("!",0) != -1)  //number pipe
			{			
				if(buf[i].find("!",0) != -1)
					err_flag = 1;			
				else
					donumpipe = 1;				

				buf[i].erase(0,1);	
				pipnum = atoi(buf[i].c_str()) ;
				buf.erase(buf.begin()+i) ;

				user[myid].line_n++;
				line = user[myid].line_n ;
				dest = total + pipnum ;					
				for(int h =0;h<line+1;h++)
				{
					if(user[myid].pipnum_arr[h] == dest)
					{
						numpipe_exist = 1 ;
						user[myid].line_n--;
						line = user[myid].line_n ;
					}
						
				}
				if(numpipe_exist != 1)	
				{							
					user[myid].pipnum_arr[line] = dest ;				
					pipe(user[myid].numpiper[line]) ;
				}								
			}
		}
	    
	
		if (buf[0] == "printenv" ){
			val = getenv(buf[1].c_str()) ;
			if(val != NULL)
				cout << val << endl ;
		}
		else if (buf[0] == "setenv" ){
			string p_name = buf[1];
			string v_name = buf[2];
			int path_exist =0;
			for(int i=0;i<user[i].Path.size();i++){
				if(buf[1]==user[myid].Path[i]){
					user[myid].variable_name[i] = buf[2] ;
					setenv(user[myid].Path[i].c_str(),user[myid].variable_name[i].c_str(), 1);
					path_exist = 1 ;
 				}
			}
			if(path_exist<1){
				
				user[myid].Path.push_back(buf[1]);
				user[myid].variable_name.push_back(buf[2]) ;
				setenv(buf[1].c_str(),buf[2].c_str(), 1);
				
					
				
			}					
		}
		else if(buf[0]=="exit")
			exit(0);
		else {											
			if (buf.size()>2 && pipe_flag > 0)  //pipe
			{			
				int pipe_sum =1 ;
				int pid ;
				int fid =-10;	
				for(int i=0;i < buf.size();i++)
					if(buf[i] == "|")
						pipe_sum += 1 ;
				int pipetest[5000][2];
				for(int k =0 ; k < line+1; k++)
					{
						if(user[myid].pipnum_arr[k] == total)
						{
							close(user[myid].numpiper[k][1]) ;	
							//cerr << "num" << user[myid].numpiper[k][1] << endl ;
						}
								
					}
				for(int i = 0 ;i < pipe_sum ; i++)
				{	
					pipe(pipetest[i]) ; 
					string redirect_name;
					
					pid = fork() ; 
					while(pid < 0)
					{
						pid = fork() ;
					}
					if( pid == 0 )
					{	
									
						int arg_num = 0;
						string arg1 = buf[0] ;
						string arg2 ;					
						if (buf[1]=="|")
							arg_num = 1 ;			
						else if(buf[2]=="|")
						{
							arg2 = buf[1] ;
							arg_num = 2 ;
						}		
						else
						{
							if(buf.size()==2)
							{
								arg2 = buf[1] ;
								arg_num = 2 ;
							}
							else if(buf.size()==1)
								arg_num = 1 ;						
						}
	
						int dev_nul = -10;
						if( i == 0 ){
							if(redirect_flag == 1 )
							{	
								for(int k =0;k<buf.size();k++)
									if(buf[k]==">")	
										redirect_name = buf[k+1] ;
								fid = open(redirect_name.c_str(),O_TRUNC | O_RDWR | O_CREAT,S_IRUSR | S_IWUSR) ;								
								if(fid == -1)
									cout << "error" ;
								close(fid) ;															
							}
							if (clear_i==1 ){
								dev_nul = open("/dev/null",O_RDONLY);
								dup2(dev_nul,0);
								close(dev_nul);
								
							}else if(send_id <31 &&send_id > -1 && clear_i!=1) //user pipe to me <N
							{											
								dup2(user_pipe[send_id][myid][0],0);
							}	
							for(int h =0 ; h<line+1 ; h++)
							{
								if(user[myid].pipnum_arr[h] == total)
								{							
									dup2(user[myid].numpiper[h][0],0) ;								
									break ;
								}										
							}		
							//cout << arg_num << endl ;							
							dup2(pipetest[0][1], 1) ;
							//cerr << "dup" << endl ;
							for(int j=0;j<=i;j++)
							{
								close(pipetest[j][1]) ;
								close(pipetest[j][0]) ;	
							}				
								
							if ( arg_num == 2 )
							{						
								char* arg[3] = { strdup(arg1.c_str()),strdup(arg2.c_str()),NULL };
								if(execvp(arg[0], arg)<0)
								{
									cerr  << "Unknown command: [" << arg[0] << "]." <<endl ;
									exit(0) ;
								}
							}
							else
							{									
								char* arg[2] = { strdup(arg1.c_str()),NULL };
								if(execvp(arg[0], arg)<0)
								{
									cerr  << "Unknown command: [" << arg[0] << "]." <<endl ;
									exit(0) ;
								}
							}											
						}
						else if( i > 0 && i < pipe_sum-1){		
														
							dup2(pipetest[i-1][0], 0);														
							dup2(pipetest[i][1], 1);							
							for(int j=0;j<=i;j++)
							{
								close(pipetest[i-1][0]) ;
								close(pipetest[i][1]) ;
							}
							if ( arg_num == 2 )	{								
								char* arg[3] = { strdup(arg1.c_str()),strdup(arg2.c_str()),NULL };
								if(execvp(arg[0], arg)<0)
								{
									cerr  << "Unknown command: [" << arg[0] << "]." <<endl ;
									exit(0) ;
								}
							}
							else{									
								char* arg[2] = { strdup(arg1.c_str()),NULL };
								if(execvp(arg[0], arg)<0)
								{
									cerr  << "Unknown command: [" << arg[0] << "]." <<endl ;
									exit(0) ;
								}
							}									
						}			
						else if( i == pipe_sum-1 )
						{
						
							dup2(pipetest[i-1][0], 0);
							//cerr << "in" << pipetest[i-1][0] << endl ;
							if(redirect_flag == 1 )
							{					
								fid = open(buf[2].c_str(),O_TRUNC | O_RDWR | O_CREAT,S_IRUSR | S_IWUSR) ;
								dup2(fid,1) ;								
								if(fid == -1)
									cout << "error" ;
								//cerr <<"f"<< fid << endl ;
								close(fid) ;																		
							}
							if(donumpipe == 1 || err_flag==1) // number pipe
							{								
									for(int k=0;k < line+1 ; k++)
									{
										if(user[myid].pipnum_arr[k] == dest)
										{									
											dup2(user[myid].numpiper[k][1],1);	
											if(err_flag==1)
												dup2(user[myid].numpiper[k][1],2);							
				
											break ;
										}																	
									}															
							}
							
							if(pip_to<31 && pip_to > -1 && clear_o!=1) // >pip_to
							{                   
								dup2(user_pipe[myid][pip_to][1],1);	
								//cerr << "close pipto " << myid << pip_to<<user_pipe[myid][pip_to][1] << endl ;
								close(user_pipe[myid][pip_to][1]) ;	
								
							}else if(clear_o==1){
								dev_nul = open("/dev/null",O_WRONLY);
								dup2(dev_nul,1);
								close(dev_nul);								
							}	

                	
							for(int j=0;j<=i;j++)
							{
								close(pipetest[j][1]) ;
								close(pipetest[j][0]) ;
							}
							
							for(int k=0;k < line+1 ; k++)
							{					
								//cerr << user[myid].numpiper[k][0] << user[myid].numpiper[k][1] << endl ;						
								close(user[myid].numpiper[k][0]) ;
								close(user[myid].numpiper[k][1]) ;
							}
							if ( arg_num == 2 )
							{				
											
								char* arg[3] = { strdup(arg1.c_str()),strdup(arg2.c_str()),NULL };							
								if(execvp(arg[0], arg)<0)
								{
									cerr  << "Unknown command: [" << arg[0] << "]." <<endl ;
									exit(0) ;
								}
							}
							else
							{								
								char* arg[2] = { strdup(arg1.c_str()),NULL };
								if(execvp(arg[0], arg)<0)
								{
									cerr  << "Unknown command: [" << arg[0] << "]." <<endl ;
									exit(0) ;
								}
							}														
						}
					}
					else {		
						if(i == 0){	
							close(pipetest[0][1]) ;	
							if(send_id <31 &&send_id > -1 && clear_i !=1)
							{
								close(user_pipe[send_id][myid][0]);
								user_pipe[send_id][myid][0] = -10 ;
								user_pipe[send_id][myid][1] = -10 ;
							}
																						
							for(int k =0 ; k < line+1 ; k++)
								if(user[myid].pipnum_arr[k] == total)
									close(user[myid].numpiper[k][0]) ;		
						}
						else if(i == pipe_sum-1){
							close(pipetest[i-1][0]) ;							
							if(pip_to<31 &&pip_to > -1)              
								close(user_pipe[myid][pip_to][1]) ;								
						}
						else{
							close(pipetest[i-1][0]) ;
							close(pipetest[i][1]) ;
						}
			
						if(i == pipe_sum-1 && donumpipe!=1 && err_flag !=1 && pip_to ==-1)
						{
							int status;
							waitpid(pid,&status,0);
						}
													
					}	

					for(int k =0;k <buf.size();k++)	{
						if(buf[k] == "|" ){
							buf.erase(buf.begin(),buf.begin()+k+1) ;			
							break ;
						}				
					}
				}	
			}							
			else  //simple 
			{			
				for(int k =0 ; k < line+1; k++)
				{
					if(user[myid].pipnum_arr[k] == total)
					{
						close(user[myid].numpiper[k][1]) ;	
					}
							
				}
				int dev_nul=-10;	
				int fid =-10;
				pid = fork();
				while (pid<0)
					pid = fork() ;
				if (pid == 0)
				{	
					string arg1  = buf[0];
					for(int h =0;h<line+1;h++)
					{						
						if(user[myid].pipnum_arr[h] == total)
						{													
							dup2(user[myid].numpiper[h][0],0) ;
							break ;
						}										
					}
				
					if(send_id <31 &&send_id > -1 && clear_i!=1) //user pipe to me <N
					{											
						//cout << "user_pipe["<<send_id<<"]["<<myid <<"][0]: " << user_pipe[send_id][myid][0] <<endl ;
                       /* if(self0 > -1)
							dup2(self0,0);
						else */
							dup2(user_pipe[send_id][myid][0],0);
					}		
					else if (clear_i==1)
					{
						dev_nul = open("/dev/null",O_RDONLY);
						dup2(dev_nul,0);
						//close(dev_nul);
						//cout << dev_nul <<endl ;
					}
					
					if(redirect_flag == 1 ) //direct
					{
						for(int i;i<buf.size();i++){
							if(buf[i]==">"){								
								buf.erase(buf.begin()+i) ;
								fid = open(buf[i].c_str(),O_TRUNC | O_RDWR | O_CREAT,S_IRUSR | S_IWUSR) ;
								dup2(fid,1);			
								close(fid);
								buf.erase(buf.begin()+i) ;
								break;
							}
						}					
						
					}
					
					if(donumpipe==1)
					{							
						for(int h=0;h < line+1 ; h++)
						{							
							if(user[myid].pipnum_arr[h] == dest)
							{												
								dup2(user[myid].numpiper[h][1],1);
								//cerr << user[myid].numpiper[h][1] << endl ;
								break ;
							}																		
						}				
					}
					if(err_flag==1)
					{
						for(int k=0;k < line+1 ; k++)
						{							
							if(user[myid].pipnum_arr[k] == dest)
							{								
								dup2(user[myid].numpiper[k][1],1);	
								dup2(user[myid].numpiper[k][1],2);
								break ;
							}																		
						}				
					}
					if(clear_o == 1){
						dev_nul = open("/dev/null",O_WRONLY);
						dup2(dev_nul,1);						
					}else if(pip_to<31 &&pip_to > -1 && clear_o!=1) // >pip_to
					{   
						/*if(self1 > -1)
							dup2(self1,1);
						else   */             
                        	dup2(user_pipe[myid][pip_to][1],1);	
						close(user_pipe[myid][pip_to][1]) ;	
						//cerr << "close pipto " << 	user_pipe[myid][pip_to][1] << endl ;
					}
					if(dev_nul!=-10)
						close(dev_nul);
					for(int k=0;k < line+1 ; k++)
					{
						close(user[myid].numpiper[k][0]) ;
						close(user[myid].numpiper[k][1]) ;
						//cerr << "close numpip" << user[myid].numpiper[k][0] <<user[myid].numpiper[k][1] << endl ;
					}		
					if(send_id <31 &&send_id > -1) //user pipe to me <N
					{

						close(user_pipe[send_id][myid][0]);
						//close(user_pipe[send_id][myid][1]);
						//cerr << "close user_s" << user_pipe[send_id][myid][0] <<user_pipe[send_id][myid][1] << endl ;
					}					
					
					if(buf.size()==1){
						char* arg[2] = { strdup(arg1.c_str()),NULL };						
						if(execvp(arg[0], arg) < 0)
						{
							cerr  << "Unknown command: [" << arg[0] << "]." <<endl ;
							exit(0) ;
						}		
						
					}
					else if(buf.size() == 3){
						string arg2 =buf[1];
						string arg3 =buf[2];						
						char* arg[4] = { strdup(arg1.c_str()),strdup(arg2.c_str()),strdup(arg3.c_str()),NULL };
						if(execvp(arg[0], arg)<0){
							cerr  << "Unknown command: [" << arg[0] << "]." <<endl ;
							exit(0);
						}						
					}
					else{
						string arg2 =buf[1];
						char* arg[3] = { strdup(arg1.c_str()),strdup(arg2.c_str()),NULL };
						if(execvp(arg[0], arg)<0){
							cerr  << "Unknown command: [" << arg[0] << "]." <<endl ;
							exit(0);
						}							
					}							
				}
				else
				{
					if(donumpipe==1 || err_flag==1 || pip_to>-1){
						int status;
						waitpid(pid,&status,WNOHANG);
					}
					else{
						
						int status;
						waitpid(pid,&status,0);
					
					}
					
					
					for (int i =0;i<line+1;i++)
					{
						if(user[myid].pipnum_arr[i]==total)
						{
							close(user[myid].numpiper[i][0]) ;
							//cerr << "close num" << user[myid].numpiper[i][0] << endl ;
						}
								
					}
					if(pip_to<31 &&pip_to > -1 && myid!=pip_to) // >pip_to
					{              
						close(user_pipe[myid][pip_to][1]) ;	
						//cerr << "p close pipp " << myid << pip_to<<	user_pipe[myid][pip_to][1] << endl ;
					}/*else{
						close(self1);
					}*/
					if(send_id <31 &&send_id > -1)	//<
                    {
                        close(user_pipe[send_id][myid][0]);
						user_pipe[send_id][myid][0] = -10 ;
						user_pipe[send_id][myid][1] = -10 ;
						
                    }																						
				}	
			}	
		}
}
void splitStr2Vec(string s, vector<string>& buf)
{
	int current = 0;
	int next;
	while (1)
	{		
		next = s.find_first_of(" ", current);
		if (next != current)
		{
			string tmp = s.substr(current, next - current);
			if (tmp.size() != 0)
				buf.push_back(tmp);
		}
		if (next == string::npos) break;
		current = next + 1;
	}
}
const char welcome_string[] =	"****************************************\n"
				"** Welcome to the information server. **\n"
				"****************************************\n";
void err_dump(char* msg){
	perror(msg);
	exit(1);
}
int fd_to_index(int fd){
	int i;
	for(i = 1 ; i < 31 ; i++){
		if(user[i].fd == fd){
			return i;
		}
	}
	return -1;
}
void user_in(struct sockaddr_in cli_addr, int newsockfd)
{   
    char port_buff[5] ;
    int  next_user;
    for(int i=1;i<31;i++)
    {
        if(user[i].usage==false)
        {
            next_user = i ;
            user[i].usage = true ;
			user[i].id = i ;
			user[i].fd  = newsockfd ;
			
			strcpy(user[i].ip,inet_ntoa(cli_addr.sin_addr));
			//sprintf(port_buff, "%hu", ntohs(cli_addr.sin_port));
			int port = ntohs(cli_addr.sin_port) ;
			sprintf(port_buff,"%d",port) ;
			strcat(user[i].ip, ":");
			strcat(user[i].ip, port_buff);

			strcpy(user[i].name, "(no name)");
            break ;
        }                 
    }    
}
void delete_user(int fd)
{	
    int id = fd_to_index(fd) ;
    user[id].usage = false ;
    user[id].id = -1;
    user[id].fd = -1 ;
    memset(user[id].name, 0, sizeof(user[id].name));
	memset(user[id].ip, 0, sizeof(user[id].ip));   
}
int passiveTCP(int port, int qlen){
	int sockfd;
	struct sockaddr_in  serv_addr;
    
	int flag =1;
    int len = sizeof(int) ;
	
	if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
		err_dump("server:can't open stream socket\n");
	}
	memset((char*)&serv_addr, 0, sizeof(serv_addr));
	//bzero((char* )&serv_addr, sizeof(serv_addr));
	serv_addr.sin_family		= AF_INET;
	serv_addr.sin_addr.s_addr	= htonl(INADDR_ANY);
	serv_addr.sin_port			= htons(SERV_TCP_PORT);
    
    if (setsockopt(sockfd,SOL_SOCKET,SO_REUSEADDR,&flag,len)<0)
    {
        err_dump("setsockopt");
        exit(1);
    }

	if(bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0){
		err_dump("server:can't bind local address\n");
	}
	listen(sockfd, qlen);
	return sockfd;
}
void login_tell_all(int newsockfd)
{
    int index = fd_to_index(newsockfd) ;
    char message[5000] ;
    sprintf(message, "*** User '%s' entered from %s. ***\n", user[index].name, user[index].ip);   
    broadcast(newsockfd,message);
}
void env_init(int fd)
{
	int id = fd_to_index(fd) ;
	user[id].Path.clear() ;
	user[id].variable_name.clear() ;
	user[id].Path.push_back("PATH") ;
	user[id].variable_name.push_back("bin:.") ;
}
void logout_tell_all(int newsockfd)
{
    int id = fd_to_index(newsockfd) ;
    char str[100] ;
    sprintf(str, "*** User '%s' left. ***\n", user[id].name); 
	
    broadcast(newsockfd,str);
}
void broadcast(int newsockfd,char* message)
{
    int i; 
	for(i = 1 ; i < 31  ; i++){
		if(user[i].usage!=false)
			tell(user[i].fd, message);
	}
}
void who(int fd) 
{
    char msg[5000];
    write(fd, "<ID>\t<nickname>\t<IP:port>\t<indicate me>\n", strlen("<ID>\t<nickname>\t<IP:port>\t<indicate me>\n"));
   	for(int i = 1 ; i < 31  ; i++){
		if(user[i].usage!=false){			
		sprintf(msg, "%d\t%s\t%s", user[i].id, user[i].name, user[i].ip);
        if(user[i].fd == fd){
			strcat(msg, "\t<-me");
		}
		strcat(msg, "\n");
		tell(fd, msg);
		}
	}
}
void tell (int fd,char* message)
{
   write(fd, message, strlen(message));
}
void yell(int fd,char* recv_buf)
{
    int id = fd_to_index(fd) ;
    char* temp ;
	int k =0;
    temp =strtok(recv_buf," ") ;
    k = strlen(temp) +1 ;
    char str[5000];
    sprintf(str, "*** %s yelled ***: %s\n", user[id].name, temp+k);
    broadcast(fd,str);

}
int client_input(int sockfd,int msockfd)
{
    while(1){
		char c ;
		int read_return ;
   int enter = 10  ;
		char  recv_buf[MAXLINE];
    memset( recv_buf,'\0',MAXLINE);
		int i;
		for(i=0;i<MAXLINE;i++){
      
			if((read_return=read(sockfd,&c,1))==1){
				if(c=='\n'){
         // cout << n << endl ;
					break ;
                }
     else if(c=='\r'){
     if(i==0)
       enter = 1 ;
       break  ;
     }
				else{
					recv_buf[i] = c ;

        }
				
			}else if (read_return <0){
      cerr << "read error" << endl;
      }
		}
   if( enter == 1)
   {
  // cout << "f" << endl ;
     write(sockfd, "% ", 2);
  	return 0 ;
    }
   else if(parse(sockfd,recv_buf,msockfd)==1) //exit
				return 1 ;
		return i ;
	}
	return 0;
}
void recover(int stdin,int stdout,int stderror)
{
	dup2(stdin,0);
	dup2(stdout,1) ;
	dup2(stderror,2) ;
	if(!cin.good())
		cin.clear();
}
void rename(int fd,char* recv_buf)
{
    char* temp ;
    temp =strtok(recv_buf," ") ;
    temp = strtok(NULL," ") ;
    char msg[5000];
    for(int i = 1 ; i < 31 ; i++){
        if(user[i].fd != fd && user[i].usage == true&&strcmp(user[i].name, temp) == 0){
			sprintf(msg, "*** User '%s' already exists. ***\n", temp);
			tell(fd, msg);
			return;
        }
    }
    
    int id = fd_to_index(fd);
    strcpy(user[id].name,temp) ;
    
    sprintf(msg, "*** User from %s is named '%s'. ***\n", user[id].ip, temp);
    broadcast(fd,msg) ;
}
void tell_deal(int sockfd,char* recv_buf)
{
    char* temp ;
    char copy[5000] ;
    strcpy(copy,recv_buf) ;
    int k = 0,j=0;
    temp =strtok(recv_buf," ") ;
    k += strlen(temp) + 1 ;
    temp =strtok(NULL, " ") ;
    k += strlen(temp) + 1 ;
    int listen_id = atoi(temp) ;    
    int id = fd_to_index(sockfd) ;
    char str[5000] ;
	if(listen_id>30){
		sprintf(str,"*** Error: user #%d does not exist yet. ***\n", listen_id);
		tell(user[id].fd,str) ;
	}
	else if(user[listen_id].usage !=true){
		sprintf(str,"*** Error: user #%d does not exist yet. ***\n", listen_id);
   		tell(user[id].fd,str) ;
	}
	else{
		sprintf(str,"*** %s told you ***: %s\n", user[id].name,copy+k);
   		tell(user[listen_id].fd,str) ;
	}
    
}

int parse(int sockfd,char* recv_buf,int msockfd)
{
   int myid = fd_to_index(sockfd) ;
	dup2(sockfd, 0);
    dup2(sockfd, 1);
    dup2(sockfd, 2);
	if(strlen(recv_buf)<1){
	//	write(sockfd, "% ", 2);
    	return 0 ;
	}
    if(strncmp(recv_buf,"exit",4)==0)
        return 1 ;
    else if(strncmp(recv_buf,"who",3)==0){
    user[myid].total_n ++;
	    who(sockfd);

	}
    else if(strncmp(recv_buf,"name",4)==0){
    user[myid].total_n ++;
        rename(sockfd,recv_buf) ;

	}
    else if(strncmp(recv_buf,"yell",4)==0){
    user[myid].total_n ++;
        yell(sockfd,recv_buf) ;  

	}
    else if(strncmp(recv_buf,"tell",4)==0){
     user[myid].total_n ++;
        tell_deal(sockfd,recv_buf) ;

	}
    else //npshell
    {       
		
        npshell(sockfd,recv_buf); 
        
		/*close(0);
		close(1);
		close(2);*/
    } 
	write(sockfd, "% ", 2);
    return 0 ;
}
void userpipe_init()
{
	for(int i=1;i<31;i++)
		for(int j=1;j<31;j++)
			for(int k=0;k<2;k++)
				user_pipe[i][j][k] = -10;
}
void userin_init(int fd)
{
	int id = fd_to_index(fd) ;
	for(int k=0;k<5000;k++)
	{
		user[id].numpiper[k][0] = -10;
		user[id].numpiper[k][1] = -10;
	}
	for(int k=0;k<5000;k++)
	{
		user[id].pipnum_arr[k] = -10;
	}
	
}
void remove_remain_fifo(int fd){
	int myid = fd_to_index(fd) ;

	for( int i = 1 ; i < 31 ; i++){
		if(user_pipe[i][myid][0] > 0){
		
			close(user_pipe[i][myid][0]);
			user_pipe[i][myid][0] = -10;
			close(user_pipe[i][myid][1]);
			user_pipe[i][myid][1] = -10;
		}
		if(user_pipe[myid][i][0] > 0){
			close(user_pipe[myid][i][0]);
			user_pipe[myid][i][0] = -10;
			close(user_pipe[myid][i][1]);
			user_pipe[myid][i][1] = -10;
		}
	}
}
int main(int argc, char* argv[])
{
 char ** envir = environ;
    
    while(*envir)
    {
        vecenvir.push_back(*envir);
        envir++;
    }
   // return 0;
	userpipe_init() ;

	user[0].usage=false;
	int msockfd;//master sock
	struct sockaddr_in cli_addr;
	fd_set rfds; // read file descriptor set
	fd_set afds; // active file descriptor set
	int fd, nfds;
	int stdin = dup(0);
	int stdout= dup(1) ;
	int stderror = dup(2) ;
	socklen_t alen; // client address length
	SERV_TCP_PORT = atoi(argv[1]) ;
	msockfd = passiveTCP(SERV_TCP_PORT, 5);
	nfds = getdtablesize();
	FD_ZERO(&afds);
	FD_SET(msockfd, &afds);
	signal(SIGCHLD,SIG_IGN);
 
	while(1)
	{
		memcpy(&rfds, &afds, sizeof(rfds));
 
		if(select(nfds, &rfds, (fd_set *)0, (fd_set *)0, (struct timeval *)0) < 0){
			err_dump("select error");
		}
		else{
			if(FD_ISSET(msockfd, &rfds)){
				int newsockfd;
				alen = sizeof(cli_addr);
				newsockfd = accept(msockfd, (struct sockaddr *)&cli_addr, &alen);
				if(newsockfd < 0){
					err_dump("accept error");
				}
				FD_SET(newsockfd, &afds);
				//add a user           
				user_in(cli_addr, newsockfd);
				userin_init(newsockfd) ;
				//write welcome message and prompt
				env_init(newsockfd) ;
				write(newsockfd, welcome_string, strlen(welcome_string) * sizeof(char)); 
				login_tell_all(newsockfd); //tell all iam in
				write(newsockfd, "% ", 2);
				
			}
			//proccess requests
			for(fd = 0 ; fd < nfds ; fd++){			
     
				if(fd != msockfd && FD_ISSET(fd, &rfds)){
					// client type "exit"              
					if(client_input(fd,msockfd) == 1){
						logout_tell_all(fd);
						remove_remain_fifo(fd);
						delete_user(fd) ;
			
						
						close(fd);
						FD_CLR(fd, &afds);
					}
					recover(stdin,stdout,stderror);
				}
				
			}
		}
		
    }

}