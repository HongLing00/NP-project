#include <cstdlib>
#include <iostream>
#include <memory>
#include <utility>
#include <string.h>
#include <boost/algorithm/string.hpp>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <signal.h>
#include <fstream>
#include <stdio.h>
#define MAXSIZE 10000
#define MAXLAN 65536
using namespace std;
using boost::asio::ip::tcp;
boost::asio::io_context context;

class session
  : public std::enable_shared_from_this<session>
{
public:
  session(tcp::socket socket)
    : socket_(std::move(socket))
  {
  }
  void start()
  {
    read_s4() ;
  }

private:
  void resolve_handler(string dstip,string dstport){
    auto self(shared_from_this());
    tcp::resolver::query query(dstip,dstport);
    boost::system::error_code ec ;
    tcp::resolver::iterator it = resolver.resolve(query,ec) ;
    /*if (ec) 
        cout << "resolve fail"<<endl;*/
    dest_socket_.connect(*it, ec);
    if(ec)
      {
        cout << ec.message()<< endl;
        //cout << "connect fail"<<endl;  
      }  
      do_reply_connect()  ;
  }/*
  void connect(const tcp::resolver::iterator& it) {
    dest_socket_.async_connect(
        *it, [this](const boost::system::error_code& ec) {
          if (!ec) {
            do_reply_connect();
            cout << "connect_aa" <<endl ;
          }else 
          cerr << ec.message() ;
        });
  }*/
  void read_src(){
    auto self(shared_from_this());
    socket_.async_read_some(boost::asio::buffer(src_buf,MAXLAN),
        [this, self](boost::system::error_code ec, std::size_t length)
        {
          if(!ec){
            write_dest(length);
          }else {
            socket_.close();
            dest_socket_.close();
          }  
                 
        });
  }
  void write_dest(size_t length){
     auto self(shared_from_this());
     async_write(dest_socket_,boost::asio::buffer(src_buf, length),
        [this, self](boost::system::error_code ec, std::size_t length)
        {     
          if(!ec)  
            read_src() ;
        });
  }
  void read_dest(){
    auto self(shared_from_this());
    dest_socket_.async_read_some(boost::asio::buffer(des_buf,MAXLAN),
        [this, self](boost::system::error_code ec, std::size_t length)
        {
          if(!ec){
            write_src(length);
          }else {
            socket_.close();
            dest_socket_.close();
          }       
        });
  }
  void write_src(size_t length){
     auto self(shared_from_this());
    async_write(socket_,boost::asio::buffer(des_buf, length),
        [this, self](boost::system::error_code ec, std::size_t length)
        {       
          if(!ec)
            read_dest() ;
          
        });
  }
  void do_reply_connect(){
    auto self(shared_from_this());
    reply[0] = 0;
    if(firewall==1)
      reply[1] = 90;
    else
    {
     reply[1] = 91;
    }
    
    for(int i =2;i<8;i++)
      reply[i] = request[i];
    socket_.async_send(boost::asio::buffer(reply,8),
                [this, self](boost::system::error_code ec, std::size_t ){
                  if(!ec)
                  {
                  }
                    
                });
    if(firewall==1){
      read_src();
       read_dest();
    }

  }
  void do_reply_bind(){
    auto self(shared_from_this());
    tcp::endpoint endpoint(tcp::v4(), 0);
    acceptor_bind.open(endpoint.protocol());
    acceptor_bind.set_option(tcp::acceptor::reuse_address(true));
    acceptor_bind.bind(endpoint);
    acceptor_bind.listen();
    u_short dst__port = acceptor_bind.local_endpoint().port();
    reply[0] = 0;
    if(bindwall == 1)
      reply[1] = 90;
    else
    {
      reply[1] = 91;
    }
    
    reply[2] = (u_char)(dst__port/256) ;
    reply[3] = (u_char)(dst__port%256) ;
    for(int i =4;i<8;i++)
      reply[i] = 0;
    socket_.async_send(boost::asio::buffer(reply,8),
                [this, self](boost::system::error_code ec, std::size_t ){
                  if(!ec)
                  {
                  }                  
                });
    if(bindwall ==1){
      acceptor_bind.accept(dest_socket_);
      socket_.async_send(boost::asio::buffer(reply,8),
                  [this, self](boost::system::error_code ec, std::size_t ){
                    if(!ec){
                    }                  
                  });
     // cout <<"reply 2" << endl ;
      read_src();
      read_dest();
    }
  }
  void print_info(string dstip,string dstport){
    cout << "<S_IP>: " << socket_.local_endpoint().address().to_string() << endl
       << "<S_PORT>: " << to_string(socket_.local_endpoint().port()) << endl
       << "<D_IP>: " << dstip << endl
       << "<D_PORT>: " << dstport << endl
       << "<Command>: " << ((cd == 1) ? "CONNECT" : "BIND") << endl ;
       if(cd==1)
         cout << "<Reply>: " << ((firewall==1) ? "Accept" : "Reject")<< endl<< endl;
       else if(cd==2)
        cout << "<Reply>: " << ((bindwall==1) ? "Accept" : "Reject")<< endl<< endl;
  }

  void read_s4(){
    auto self(shared_from_this());
    socket_.async_read_some(boost::asio::buffer(request, MAXSIZE),
        [this, self](boost::system::error_code ec, std::size_t length)
        {       
        });
   
    
    cd = request[1] ;
    //cout << "cd" << to_string(cd) << endl ;
    string dstport="";
    dstport = to_string(request[2] * 256 + request[3]);
    string dstip="";
    dstip = to_string(request[4]) + "." + to_string(request[5]) + "." + to_string(request[6]) + "." + to_string(request[7]);
    //cout << dstip << endl ;
    if(to_string(request[4])=="0" &&to_string(request[5])=="0" &&to_string(request[6])=="0"&&to_string(request[7])!="0"){
     // cout << "4a" << endl ;
      char* user_id = (char*)request + 8;
      size_t user_id_len = strlen(user_id) ;     
      char* domain_name = user_id + user_id_len + 1;

      tcp::resolver::iterator iter = resolver_.resolve(tcp::resolver::query(domain_name,""));       
      for(tcp::resolver::iterator it=iter;it!= tcp::resolver::iterator();it++){
        tcp::endpoint ep = *it;         
       // cout << ep.address() << endl ;
        if(ep.address().is_v4()){
          dstip = ep.address().to_string();
        }
      }  
    }
    char spliter = '.';
    string token;
    istringstream iss(dstip);
    string ip_deal[4]; 
    for(int i =0;i<4;i++){
      getline(iss, token, spliter);
      ip_deal[i] = token ;
    }
    ifstream filestream("socks.conf");

    firewall = 0 ;
    bindwall = 0 ;
    string line;
    string per, mode,ip_;
    if(filestream){
      while(filestream >> per >> mode >> ip_){
        char spliter = '.';
        istringstream iss(ip_);
        string token;
        for(int i =0;i<4;i++){
          getline(iss, token, spliter);
          if(token!="*"){
            if(token != ip_deal[i]){
              //cout <<  token << to_string(request[i+4]) << endl ;
              break ;
            }           
          }else{
            if(mode == "c")
              firewall = 1 ;
            else if (mode == "b")
              bindwall = 1 ;
          }
         }
      }
    }    
    if(cd== 1){ //connect
      if(firewall == 1){
        resolve_handler(dstip,dstport) ;    
      }else{
        do_reply_connect();
      }
    }
    if(cd == 2){
      do_reply_bind();
     // cout << "bind " << endl ;
    }
    if(cd!=0)
      print_info(dstip,dstport);
    

  }
  unsigned char cd ;
  unsigned char request[MAXSIZE];
  unsigned char reply[8];
  array<unsigned char, MAXLAN> des_buf{};
  array<unsigned char,MAXLAN> src_buf{};
  int firewall ;
  int bindwall ;
  tcp::resolver resolver{context};
  tcp::resolver resolver_{context};
  tcp::acceptor acceptor_{context};
  tcp::acceptor acceptor_bind{context};
  tcp::socket socket_{context}; 
  tcp::socket dest_socket_{context};
  tcp::endpoint endpoint_;
};

class server
{
public:
  server(unsigned short port)
    : acceptor_(context, tcp::endpoint(tcp::v4(), port))
  {
    do_accept();
  }

private:
  void do_accept()
  {
    acceptor_.async_accept(
        [this](boost::system::error_code ec, tcp::socket socket)
        {
          if (!ec){
              context.notify_fork(boost::asio::io_service::fork_prepare);
              if (fork() == 0){
                context.notify_fork(boost::asio::io_context::fork_child);
                acceptor_.close();
                try
                {
                  std::make_shared<session>(std::move(socket))->start();
                    
                    socket.close();
                }
                catch (std::exception& e)
                {
                    socket.close();
                }
              }else{
                context.notify_fork(boost::asio::io_context::fork_parent);
                socket.close();
                do_accept() ;
              }
          }
          else{
              cerr << "Accept error: "<< endl;
              do_accept() ;
          }
        });
  }
  tcp::acceptor acceptor_;
};
int main(int argc, char* argv[])
{ 
  signal(SIGCHLD,SIG_IGN);
  try
  {
    if (argc != 2)
    {
      std::cerr << "Usage: socks_server <port>\n";
      return 1;
    }
    server s(std::atoi(argv[1]));
    context.run();
  }
  catch (std::exception& e)
  {
    std::cerr << "Exception: " << e.what() << "\n";
  }

  return 0;
}