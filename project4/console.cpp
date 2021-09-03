#include <boost/algorithm/string.hpp>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <fstream>
#include <iostream>
#include <string>
using namespace boost::asio;
using namespace boost::asio::ip;
using namespace std;
io_context context;
struct Hosts {
        string hostname;
        string port;
        string file;
        string s4;
        string s4_port;
        bool usage(){
            return !hostname.empty() ;
        }
}host[5];
void replace(string &data) {
    boost::replace_all(data, "&", "&amp;");
    boost::replace_all(data, "\"", "&quot;");
    boost::replace_all(data, "\'", "&apos;");
    boost::replace_all(data, "<", "&lt;");
    boost::replace_all(data, ">", "&gt;");
    boost::replace_all(data, "\n", "&NewLine;");
    boost::replace_all(data, "\r", "");
}
void output_shell(string session,string content){
    replace(content);  
    cout << "<script>document.getElementById('"<< session<< "').innerHTML += '"<< content<< "';</script>";
    std::cout.flush();
}
void output_command(string session,string content){
    replace(content);
    cout << "<script>document.getElementById('"<< session << "').innerHTML += '<b>"<< content<< "</b>';</script>";
    cout.flush();
}
string html_console(){
    string scope ;
    string pre_id ;
    string str1 = 
    "Content-type: text/html\r\n\r\n"
    "<!DOCTYPE html>\n"
    "<html lang=\"en\">\n"
    "  <head>\n"
    "    <meta charset=\"UTF-8\" />\n"
    "    <title>NP Project 3 Sample Console</title>\n"
    "    <link\n"
    "      rel=\"stylesheet\"\n"
    "      href=\"https://cdn.jsdelivr.net/npm/bootstrap@4.5.3/dist/css/bootstrap.min.css\"\n"
    "      integrity=\"sha384-TX8t27EcRE3e/ihU7zmQxVncDAy5uIKz4rEkgIXeMed4M0jlfIDPvg6uqKI2xXr2\"\n"
    "      crossorigin=\"anonymous\"\n"
    "    />\n"
    "    <link\n"
    "      href=\"https://fonts.googleapis.com/css?family=Source+Code+Pro\"\n"
    "      rel=\"stylesheet\"\n"
    "    />\n"
    "    <link\n"
    "      rel=\"icon\"\n"
    "      type=\"image/png\"\n"
    "      href=\"https://cdn0.iconfinder.com/data/icons/small-n-flat/24/678068-terminal-512.png\"\n"
    "    />\n"
    "    <style>\n"
    "      * {\n"
    "        font-family: 'Source Code Pro', monospace;\n"
    "        font-size: 1rem !important;\n"
    "      }\n"
    "      body {\n"
    "        background-color: #212529;\n"
    "      }\n"
    "      pre {\n"
    "        color: #cccccc;\n"
    "      }\n"
    "      b {\n"
    "        color: #01b468;\n"
    "      }\n"
    "    </style>\n"
    "  </head>\n"
    "  <body>\n"
    "    <table class=\"table table-dark table-bordered\">\n"
    "      <thead>\n"
    "        <tr>\n";
    for (int i = 0; i < 5; i++)   
      if (host[i].usage())
          scope += "          <th scope=\"col\">" + host[i].hostname + ":" +host[i].port + "</th>" ;

    string str2 =   
    "        </tr>\n"
    "      </thead>\n"
    "      <tbody>\n"
    "        <tr>\n" ;

    for (int i = 0; i < 5; i++)    
      if (host[i].usage()) 
        pre_id += "          <td><pre id=\"s" + to_string(i)  + "\" class=\"mb-0\"></pre></td>";

    string str3 = 
    "        </tr>\n"
    "      </tbody>\n"
    "    </table>\n"
    "  </body>\n"
    "</html>\n" ;
    return str1 + scope + str2 + pre_id + str3 ;
}
vector<string> spilt_query(const string &s){
   string query_copy = s ;
   vector<string> res;
   boost::split(res,query_copy,boost::is_any_of("&"),boost::token_compress_on);
   return res;
}
struct connect_socks
    : public std::enable_shared_from_this<connect_socks> {
    connect_socks(string session_,string sock_server,string sock_port, string file_name, string http_server,string http_port)
      : session(std::move(session_)),sock_server(sock_server),sock_port(sock_port),http_server_(http_server),http_port_(http_port)
    {
    file.open("test_case/" + file_name,ios::in);
  }
  void start()
    {
        resolve_handler();
    }
private :
  void read_handler() {
    auto self(shared_from_this());
    socket_.async_read_some(boost::asio::buffer(data_, max_length),
        [this, self](boost::system::error_code ec, std::size_t length){
            if (!ec) {           
                string data=string(data_,length);     
                output_shell(session, data);
                if (data.find("% ")!=string::npos) {
                    write_handler();
                    read_handler() ;   
                }else
                    read_handler() ;                  
            }
    });
  }
  void write_handler() {
        auto self(shared_from_this());
        string command ;
        getline(file, command);
        command += '\n' ;
        output_command(session, command);
        async_write(socket_,buffer(command.c_str(),command.length()),
                [this, self](boost::system::error_code ec, std::size_t length ) {
                }
        );
    }
    void do_connect(tcp::resolver::iterator iter) {
      auto self(shared_from_this());
        tcp::endpoint endpoint = *iter;
        socket_.async_connect(
            endpoint,
            [this, self](boost::system::error_code ec) {
                if (!ec) 
                {
                    set_sock_server();
                }
                    
            }
        );
  }
  void resolve_handler() {
      auto self(shared_from_this());
       boost::asio::ip::tcp::resolver::query q{sock_server, sock_port};
   /* auto ite = resolver.resolve(que_);
    boost::asio::connect(socket_, ite);*/
    resolver.async_resolve(q,[this, self](boost::system::error_code ec, tcp::resolver::iterator iter) {
                if (!ec) 
                    do_connect(iter);
            }
        );
  }
 void set_sock_server(){
        auto self(shared_from_this());
    resolver.async_resolve(tcp::resolver::query(http_server_, http_port_),[this, self](const boost::system::error_code &ec,
                                        tcp::resolver::iterator it) {
                             if (!ec)
                               {
                                   tcp::endpoint http_endpoint = *it;
                                   char request[9];
                                
                                    u_int port = http_endpoint.port() ;
                                    request[0] = 4 ;
                                    request[1] = 1 ;
                                    request[2] = u_char(port/256);
                                    request[3] = u_char(port%256);
                                    request[4] = http_endpoint.address().to_v4().to_bytes()[0];
                                    request[5] = http_endpoint.address().to_v4().to_bytes()[1];
	                                request[6] = http_endpoint.address().to_v4().to_bytes()[2];
	                                request[7] = http_endpoint.address().to_v4().to_bytes()[3];
                                    request[8] = 0 ;
                                     boost::asio::async_write(socket_,boost::asio::buffer(request, 9),[this, self](boost::system::error_code ec, size_t) {
                                        if(!ec){
                                        socket_.async_read_some(buffer(bytes_, 8),[this, self](boost::system::error_code ec, size_t  length ){
                                            
                                            if(bytes_[1] == 90){
                                                read_handler() ;   
                                            }else
                                            {
                                                socket_.close();
                                            }
                                            
                                        });
                                        }
                                    });
                               }
                           });

    
  }
  

  array<char, 4096> bytes_;
  string http_server_;
  string http_port_;
  string sock_server ;
  string sock_port ;
  string session;
  tcp::resolver resolver{context};
  tcp::socket socket_{context}; 
  fstream file; 
  enum { max_length = 1024 };
  char data_[max_length];
};
struct User 
  : public std::enable_shared_from_this<User> 
  {
    public:
    User(string session, string file_name, tcp::resolver::query que):
    session(std::move(session)), que(std::move(que))
    {
        file.open("test_case/" + file_name,ios::in);
    }
    void start()
    {
        resolve_handler();
    }
    private :
    void read_handler() {
    auto self(shared_from_this());
    socket_.async_read_some(boost::asio::buffer(data_, max_length),
        [this, self](boost::system::error_code ec, std::size_t length){
            if (!ec) {           
                string data=string(data_,length);     
                output_shell(session, data);
                if (data.find("% ")!=string::npos) {
                    write_handler();
                    read_handler() ;   
                }else
                    read_handler() ;                  
            }
    });
  }
  void write_handler() {
        auto self(shared_from_this());
        string command ;
        getline(file, command);
        command += '\n' ;
        output_command(session, command);
        async_write(socket_,buffer(command.c_str(),command.length()),
                [this, self](boost::system::error_code ec, std::size_t length ) {
                }
        );
    }
  void connect_handler(tcp::resolver::iterator it) {
    auto self(shared_from_this());
    socket_.async_connect(*it, [this, self](boost::system::error_code ec) {
      if (!ec)
        read_handler();
    });
  }
  void resolve_handler(){
    auto self(shared_from_this());
    resolver.async_resolve(que,[this, self](const boost::system::error_code &ec,
                                        tcp::resolver::iterator it) {
                             if (!ec)
                               connect_handler(it);
                           });
  }
  string session;
  tcp::resolver::query que;
  tcp::resolver resolver{context};
  tcp::socket socket_{context}; 
  fstream file; 
  enum { max_length = 1024 };
  char data_[max_length];

};

string env_query(string s){
    char *query = getenv(s.c_str());
    if (query==nullptr)
        return "None";
    return query;
}
int main(){
    std::string socks_server;
    std::string socks_port;
    for(auto line : spilt_query(env_query("QUERY_STRING"))){

        string s1 = line.substr(0, line.find('='));
        string s2 = line.substr(s1.size()+1) ;
        if (!s2.empty()) {
        
        if (s1 == "sh") {
            socks_server = s2;
            continue;
        } else if (s1 == "sp") {
            socks_port = s2;
            continue;
        }
        
        char alpha = s1.front();
        int pos = s1.back()-'0';
        if (alpha=='h')
            host[pos].hostname = s2;
        else if (alpha=='p')
            host[pos].port = s2;
        else if (alpha=='f')
            host[pos].file = s2;
        }
    }

    cout << html_console();
    for (size_t i = 0; i < 5; i++) {     
      if (host[i].usage()) {
        if (!socks_server.empty() && !socks_port.empty()) {
           // boost::asio::ip::tcp::resolver::query q{socks_server, socks_port};
            std::make_shared<connect_socks>("s" + to_string(i), socks_server,socks_port,host[i].file, host[i].hostname, host[i].port)
                ->start();
        }
        else{
          boost::asio::ip::tcp::resolver::query que{host[i].hostname, host[i].port};
          make_shared<User>("s" + to_string(i), host[i].file, move(que))->start();
        }
        }
    }
    context.run() ;
}