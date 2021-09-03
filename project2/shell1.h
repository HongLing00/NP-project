#include <iostream>   
#include<string>
#include<cstring>
#include<algorithm>
#include<vector>
#include <iostream>
#include <cstdlib>
#include <unistd.h>
#include <fstream>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>

using namespace std;
void splitStr2Vec(string s, vector<string>& buf);
void piper(vector<string>& buf) ;
void redirect(vector<string>& buf) ;
int pipnum_arr[5000];
int line  = -1 ;
int dest;
int status ;
int donumpipe ;
int space = 0;

void npshell()
{
  signal(SIGCHLD,SIG_IGN);
	setenv("PATH", "bin:.", 1);
	char* val;
	int pid ;
	int i,j ;
	int dir_flag = 0;
	int numpiper[5000][2] ;
	int total =0;
	signal(SIGCHLD,SIG_IGN);
	string s ;
	while(1)
	{
		cout << "% " ;
		
		vector<string> buf;
		getline(cin,s) ;
		if(s.find("\r")!=s.npos){
      	int pos = s.find('\r');
		  s.erase(pos,1) ;
    	}
		if(s == "")
			continue ;
		else 
			total ++;
		string numpi ;
		donumpipe = 0;
		splitStr2Vec(s, buf);
		int numpipe_exist = 0;
		int pipe_flag = 0 ;
		int err_flag = 0;
		int pipnum ;
		int redirect_flag = 0;
		int buf_size = buf.size();

		for(i=0;i < buf_size;i++)
		{
			if (buf[i] == ">")
				redirect_flag = 1 ;
			if(buf[i] == "|") // pipe
				pipe_flag += 1 ;
		}
		if(buf[buf_size-1].find("|",0) != -1 || buf[buf_size-1].find("!",0) != -1)  //number pipe
		{			
			if(buf[buf_size-1].find("!",0) != -1)
				err_flag = 1;			
			else
				donumpipe = 1;				

			buf[buf_size-1].erase(0,1);	
			pipnum = atoi(buf[buf_size-1].c_str()) ;
			line++;
			buf.pop_back() ;	
			dest = total + pipnum ;					
			for(int h =0;h<line+1;h++)
			{
				if(pipnum_arr[h] == dest)
				{
					numpipe_exist = 1 ;
					line -- ;
				}
					
			}
			if(numpipe_exist != 1)	
			{							
				pipnum_arr[line] = dest ;
				pipe(numpiper[line]) ;
			}								
		}
		
	
		if (buf[0] == "printenv" )
		{
			val = getenv(buf[1].c_str());
			if(val!=NULL)
				cout << val << endl;			
		}
		else if (buf[0] == "setenv" )
		{
			setenv(buf[1].c_str(), buf[2].c_str(), 1);
			val = getenv(buf[1].c_str());
		}
		else if(buf[0]=="exit")
			 exit(0);
		else 
		{
														
			if(buf.size()>2 && redirect_flag == 1 && pipe_flag == 0) //direct
				redirect(buf) ;
			else if (buf.size()>2 && pipe_flag > 0)  //pipe
			{			
				int pipe_sum =1 ;
				int pid ;
				int fid ;	
				for(int i=0;i < buf.size();i++)
					if(buf[i] == "|")
						pipe_sum += 1 ;
				int pipetest[5000][2];
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
						
							for(int h =0 ; h<line+1 ; h++)
							{
								if(pipnum_arr[h] == total)
								{							
									dup2(numpiper[h][0],0) ;								
									break ;
								}										
							}									
							dup2(pipetest[0][1], 1) ;
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
						else if( i == pipe_sum-1 )
						{
							
							dup2(pipetest[i-1][0], 0);						
							if(redirect_flag == 1 )
							{					
								fid = open(buf[2].c_str(),O_TRUNC | O_RDWR | O_CREAT,S_IRUSR | S_IWUSR) ;
								dup2(fid,1) ;								
								if(fid == -1)
									cout << "error" ;
								close(fid) ;																			
							}
							if(donumpipe == 1 || err_flag==1) // number pipe
							{								
									for(int k=0;k < line+1 ; k++)
									{
										if(pipnum_arr[k] == dest)
										{									
											dup2(numpiper[k][1],1);	
											if(err_flag==1)
												dup2(numpiper[k][1],2);							
											for(int k=0;k < line+1 ; k++)
											{													
												close(numpiper[k][0]) ;
												close(numpiper[k][1]) ;
											}
											break ;
										}																	
									}															
							}
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
					}
					else //parent
					{		
						if(i == 0)
						{	
							close(pipetest[0][1]) ;		
													
							for(int k =0 ; k < line+1 ; k++)
								if(pipnum_arr[k] == total)
									close(numpiper[k][0]) ;		
						}
						else if(i == pipe_sum-1)
						{
							close(pipetest[i-1][0]) ;
						
							for(int k =0 ; k < line+1 ; k++)
								if(pipnum_arr[k] == total+1)
									close(numpiper[k][1]) ;							
						}
						else
						{
							close(pipetest[i-1][0]) ;
							close(pipetest[i][1]) ;

						}
						
						if(i == pipe_sum-1 && donumpipe!=1 && err_flag !=1)
						{
							int status;
							waitpid(pid,&status,0) ;
						//	wait(0) ;
						
						}
						
							
					}	

					for(int k =0;k <buf.size();k++)
					{
						if(buf[k] == "|" )
						{
							buf.erase(buf.begin(),buf.begin()+k+1) ;			
							break ;
						}				
					}
				}	
			}							
			else  //simple 
			{				
				pid = fork();
				if (pid<0)
					cout << "fork error" << endl  ;
				else if (pid == 0)
				{	
					string arg1  = buf[0];
					for(int h =0;h<line+1;h++)
					{						
						if(pipnum_arr[h] == total)
						{													
							dup2(numpiper[h][0],0) ;
							break ;
						}										
					}		
					if(donumpipe==1)
					{							
						for(int h=0;h < line+1 ; h++)
						{							
							if(pipnum_arr[h] == dest)
							{												
								dup2(numpiper[h][1],1);
								break ;
							}																		
						}				
					}
					if(err_flag==1)
					{
						for(int k=0;k < line+1 ; k++)
						{							
							if(pipnum_arr[k] == dest)
							{								
								dup2(numpiper[k][1],1);	
								dup2(numpiper[k][1],2);
								break ;
							}							
											
						}				
					}
					for(int k=0;k < line+1 ; k++)
					{
						close(numpiper[k][0]) ;
						close(numpiper[k][1]) ;
					}		
										
					
					if(buf.size()==1)
					{
						char* arg[2] = { strdup(arg1.c_str()),NULL };						
						if(execvp(arg[0], arg) < 0)
						{
							cerr  << "Unknown command: [" << arg[0] << "]." <<endl ;
							exit(0) ;
						}
							
						
					}
					else if(buf.size() == 3)
					{
						string arg2 =buf[1];
						string arg3 =buf[2];						
						char* arg[4] = { strdup(arg1.c_str()),strdup(arg2.c_str()),strdup(arg3.c_str()),NULL };
						if(execvp(arg[0], arg)<0)
						{
							cerr  << "Unknown command: [" << arg[0] << "]." <<endl ;
							exit(0);
						}
							
						
					}
					else
					{
						string arg2 =buf[1];
						char* arg[3] = { strdup(arg1.c_str()),strdup(arg2.c_str()),NULL };
						if(execvp(arg[0], arg)<0)
						{
							cerr  << "Unknown command: [" << arg[0] << "]." <<endl ;
							exit(0);
						}
							
					}		
					
				}
				else
				{
					if(donumpipe==1 && err_flag==1)
					{
						int status;
						waitpid(pid,&status,WNOHANG);
					}
					else
					{
						wait(0) ;
					}
					
					for(int k =0 ; k < line+1; k++)
					{
						if(pipnum_arr[k] == total+1)
						{
							close(numpiper[k][1]) ;	
						}
							
					}
					for (int i =0;i<line+1;i++)
						if(pipnum_arr[i]==total)
							close(numpiper[i][0]) ;																						
				}	
			}	
		}
	}
}
void redirect(vector<string>& buf)
 {
 	int pid;
 	pid=fork() ;
 	int errfile =0;
 	if (pid<0)
 		cout << "fork error" << endl ;
 	else if(pid == 0)
 	{
 		int fd ;
 		string arg1 = buf[0];
 		string arg2 = buf[1];
 		if(buf[1]==">")
 		{
 			fd = open(buf[2].c_str(),O_TRUNC | O_RDWR | O_CREAT,S_IRUSR | S_IWUSR) ;
			if(fd == -1)
 				cout << "error" ;
 			dup2(fd,1);			
 			close(fd);
		
 			char* arg[2] = { strdup(arg1.c_str()),NULL };
 			if(execvp(arg[0], arg)<0)
 			{
 				cerr  << "Unknown command: [" << arg[0] << "]." <<endl ;
				exit(0);
 			}	 				
 		}			
 		else
 		{
 			fd = open(buf[3].c_str(),O_TRUNC | O_RDWR | O_CREAT,S_IRUSR | S_IWUSR) ;
			if(fd == -1)
 				cout << "error" ;
 			dup2(fd,1);
		
 			close(fd);
			//dup2(1,2);	
 			char* arg[3] = { strdup(arg1.c_str()),strdup(arg2.c_str()),NULL };
 			if(execvp(arg[0], arg)<0)	
 			{
 				cerr  << "Unknown command: [" << arg[0] << "]." <<endl ;
				exit(0);
 			}							
 		}			
 	}	
 	else
 	{
 		int status ;
 		wait(&status) ;				
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