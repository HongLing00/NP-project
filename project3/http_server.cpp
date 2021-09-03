#include <cstdlib>
#include <iostream>
#include <memory>
#include <utility>
#include <boost/asio.hpp>
#include <signal.h>
#include <stdio.h>
using boost::asio::ip::tcp;
using namespace std;
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
    do_read();
  }

private:
  
  void do_read()
  {
    auto self(shared_from_this());
    socket_.async_read_some(boost::asio::buffer(data_, max_length),
        [this, self](boost::system::error_code ec, std::size_t length)
        {
          if (!ec)
          {
            do_write();
          }
            
        });
  }
  void do_write()
  {
    auto self(shared_from_this());
    boost::system::error_code ec;
    if (!ec)
          {
            char method[1024],uri[5000],protocol[2048],host[1024],host_value[1024] ;
            sscanf(data_,"%s %s %s %s %s",method,uri,protocol,host,host_value);
            string uri_s = string(uri) ;
            string cgi_name = uri_s.substr(0, uri_s.find('?'));
            string query_s;
            if (uri_s!=cgi_name) {
                query_s = uri_s.substr(cgi_name.size() + 1);
            } else {
                query_s = "";
            }
            context.notify_fork(boost::asio::io_service::fork_prepare);
            if (fork() == 0)
            {
                context.notify_fork(boost::asio::io_context::fork_child);
                setenv("REQUEST_METHOD",method,1);
                setenv("REQUEST_URI",uri,1);    
                setenv("QUERY_STRING",query_s.c_str(),1);               
                setenv("SERVER_PROTOCOL",protocol,1);
                setenv("HTTP_HOST",host_value,1);
                setenv("SERVER_ADDR",socket_.local_endpoint().address().to_string().c_str(),1);
                setenv("SERVER_PORT",to_string(socket_.local_endpoint().port()).c_str(),1);
                setenv("REMOTE_ADDR",socket_.remote_endpoint().address().to_string().c_str(),1);
                setenv("REMOTE_PORT",to_string(socket_.remote_endpoint().port()).c_str(),1);                
              
                dup2(socket_.native_handle(),0);
                dup2(socket_.native_handle(),1);
                dup2(socket_.native_handle(),2);
                string http_ok = "HTTP/1.1 200 OK\n" ;
                socket_.async_send(boost::asio::buffer(http_ok,http_ok.length()),
                [this, self](boost::system::error_code ec, std::size_t ){
                });
                cgi_name = cgi_name.substr(1);
                char *argv[] = {nullptr};
                if (execv(cgi_name.c_str(), argv)==-1) {
                    cerr<<"error !"<<endl;                   
                }
            }
            else{
                context.notify_fork(boost::asio::io_context::fork_parent);
                socket_.close();
            }    
          }   
    }

  tcp::socket socket_;
  enum { max_length = 1024 };
  char data_[max_length];
};

class server
{
public:
  server(boost::asio::io_context& io_context, short port)
    : acceptor_(io_context, tcp::endpoint(tcp::v4(), port))
  {
    do_accept();
  }

private:
  void do_accept()
  {
    acceptor_.async_accept(
        [this](boost::system::error_code ec, tcp::socket socket)
        {
          if (!ec)
          {
            std::make_shared<session>(std::move(socket))->start();
          }

          do_accept();
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
      std::cerr << "Usage: async_tcp_echo_server <port>\n";
      return 1;
    }

    
    server s(context, std::atoi(argv[1]));
    context.run();
  }
  catch (std::exception& e)
  {
    std::cerr << "Exception: " << e.what() << "\n";
  }

  return 0;
}