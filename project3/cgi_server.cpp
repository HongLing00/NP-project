#include <array>
#include <boost/algorithm/string.hpp>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/format.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>
using namespace boost::asio;
using namespace boost::asio::ip;
using namespace std;
io_context context;
typedef std::shared_ptr<ip::tcp::socket> socket_ptr;
struct Hosts {
        string hostname;
        string port;
        string file;
        bool usage(){
            return !hostname.empty() ;
        }
};
vector<string> spilt_query(const string &s){
   string query_copy = s ;
   vector<string> res;
   boost::split(res,query_copy,boost::is_any_of("&"),boost::token_compress_on);
   return res;
}
void set_structure(string s, Hosts* host){
  
  for(auto line : spilt_query(s)){
          string s1 = line.substr(0, line.find('='));
          string s2 = line.substr(s1.size()+1) ;
          if (!s2.empty()) {
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
  
  
}
void replace(string &data) {
    boost::replace_all(data, "&", "&amp;");
    boost::replace_all(data, "\"", "&quot;");
    boost::replace_all(data, "\'", "&apos;");
    boost::replace_all(data, "<", "&lt;");
    boost::replace_all(data, ">", "&gt;");
    boost::replace_all(data, "\n", "&NewLine;");
    boost::replace_all(data, "\r", "");
}
string output_shell(string session,string content){
    replace(content);  
    return ("<script>document.getElementById('"+ session+ "').innerHTML += '"+ content+"';</script>");
}
string output_command(string session,string content){
    replace(content);
    return  ("<script>document.getElementById('"+ session + "').innerHTML += '<b>"+ content+ "</b>';</script>");
}
string panel_console(){
    string s1 ="";
    string test_case_menu ="";
    string host_menu="";
    for(int i=1;i<=10;i++)
        test_case_menu += "<option value=\"t"+to_string(i)+".txt\">t"+to_string(i)+".txt</option>\n";
    for(int i=1;i<=12;i++)
        host_menu += "<option value=\"nplinux"+to_string(i)+".cs.nctu.edu.tw\">nplinux"+to_string(i)+"</option>\n" ;
    s1 = 
    "HTTP / 1.1 200 OK\n"
    "Content-type: text/html\r\n\r\n\n"
    "<!DOCTYPE html>\n"
    "<html lang=\"en\">\n"
    "  <head>\n"
    "    <title>NP Project 3 Panel</title>\n"
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
    "      href=\"https://cdn4.iconfinder.com/data/icons/iconsimple-setting-time/512/dashboard-512.png\"\n"
    "    />\n"
    "    <style>\n"
    "      * {\n"
    "        font-family: 'Source Code Pro', monospace;\n"
    "      }\n"
    "    </style>\n"
    "  </head>\n"
    "  <body class=\"bg-secondary pt-5\">\n"
    "<form action=\"console.cgi\" method=\"GET\">\n" 
    "  <table class=\"table mx-auto bg-light\" style=\"width: inherit\">\n" 
    "    <thead class=\"thead-dark\">\n" 
    "      <tr>\n" 
    "        <th scope=\"col\">#</th>\n" 
    "        <th scope=\"col\">Host</th>\n" 
    "        <th scope=\"col\">Port</th>\n" 
    "        <th scope=\"col\">Input File</th>\n" 
    "      </tr>\n" 
    "    </thead>\n" 
    "    <tbody>\n" ;
    "<tr>\n";
    for (int i=0;i<5;i++){
        s1 +=
        "        <th scope=\"row\" class=\"align-middle\">Session "+to_string(i+1)+"</th>\n";
        s1 +=
        "        <td>\n"
        "          <div class=\"input-group\">\n"
        "            <select name=\"h"+to_string(i)+"\" class=\"custom-select\">\n" ;
        s1 += 
        "              <option></option>"+host_menu+"\n";
        s1 += 
        "            </select>\n"
        "            <div class=\"input-group-append\">\n"
        "              <span class=\"input-group-text\">.cs.nctu.edu.tw</span>\n"
        "            </div>\n"
        "          </div>\n"
        "        </td>\n"
        "        <td>\n";
        s1 += 
        "          <input name=\"p"+ to_string(i)+"\" type=\"text\" class=\"form-control\" size=\"5\" />\n";
        s1 += 
        "        </td>\n"
        "        <td>\n"
        "          <select name=\"f"+ to_string(i)+"\" class=\"custom-select\">\n" ;
        s1 +=
        "            <option></option>\n"
        "            "+test_case_menu+"\n";
        s1 +=
        "          </select>\n"
        "        </td>\n"
        "      </tr>\n" ;
    }
    s1+=    
    "              <tr>\n"
    "            <td colspan=\"3\"></td>\n"
    "            <td>\n"
    "              <button type=\"submit\" class=\"btn btn-info btn-block\">Run</button>\n"
    "            </td>\n"
    "          </tr>\n"
    "        </tbody>\n"
    "      </table>\n"
    "    </form>\n"
    "  </body>\n"
    "</html>\n";
    return s1 ;

}
string html_console( Hosts* host){
    
    string scope ;
    string pre_id ;
    string str1 = 
    "HTTP / 1.1 200 OK\n"
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

class User : public std::enable_shared_from_this<User> 
  {
    public:
    User(string session,int id_, string file_name,socket_ptr ptr,string host_,string port_):
    session(session),id(id_),ptr_(ptr),host_h(host_), port_h( port_)
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
    socket_.async_read_some(boost::asio::buffer(_data, max_length),
        [this, self](boost::system::error_code ec, std::size_t length){
            if (!ec) {
                string data=string(_data,length);  
                string data_s= output_shell(session,data) ;
 
                ptr_->async_write_some(buffer(data_s.c_str(), data_s.length()),
                                      [this, self](boost::system::error_code ec, std::size_t length) {}
                                    );
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
        
        command="" ;
        getline(file, command);
        command += '\n' ;
      async_write(socket_,buffer(command.c_str(), command.length()),
			[this, self](boost::system::error_code ec, std::size_t length) {
				if (!ec) {
          
					string data_c= output_command(session,command) ;
          ptr_->async_write_some(buffer(data_c.c_str(),data_c.length()),
                [this, self](boost::system::error_code ec, std::size_t length ) {

                }
        );
    }
  });
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
    resolver_.async_resolve(ip::tcp::resolver::query(host_h, port_h)
      ,[this, self](const boost::system::error_code &ec,
                                        tcp::resolver::iterator it) {
                             if (!ec)
                               connect_handler(it);
                           });
  }
  string session;
  string command;
  string host_h ;
  string port_h ;
  int id=-1;
  struct Hosts host ;
  socket_ptr ptr_;
  tcp::resolver resolver_{context};
  tcp::socket socket_{context}; 
  fstream file; 
  enum { max_length = 1024 };
  char _data[max_length];
};
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
            data = string(data_,length);
            do_write();
          }
            
        });
  }
  void do_write()
  {
    auto self(shared_from_this());
    boost::system::error_code ec;
    
    if(data.find("/console.cgi") != string::npos){ 
      char method[1024],uri[5000],protocol[2048],host_[1024],host_value[1024] ;
      sscanf(data_,"%s %s %s %s %s",method,uri,protocol,host_,host_value);
      string uri_s = string(uri) ;
      string cgi_name = uri_s.substr(0, uri_s.find('?'));
      string query_s;
      if (uri_s!=cgi_name) {
        query_s = uri_s.substr(cgi_name.size() + 1);
      } else {
        query_s = "";
      }
      struct Hosts host[5];
      set_structure(query_s,host) ;
      string console_ = html_console(host);

      socket_.async_send(boost::asio::buffer(console_,console_.length()),
      [this, self](boost::system::error_code ec, std::size_t ){
      });
      socket_ptr ptr = std::make_shared<ip::tcp::socket>(move(socket_));
          for (size_t i = 0; i < 5; i++) {     
            if (host[i].usage()) {
              make_shared<User>("s" + to_string(i), i,host[i].file,ptr,host[i].hostname,host[i].port)->start();
           }
          }

    }
    else if(data.find("/panel.cgi") != string::npos){
      string panel_ = panel_console();
      socket_.async_send(boost::asio::buffer(panel_,panel_.length()),
         [this, self](boost::system::error_code ec, std::size_t ){
      });
    }   
  }
  string data;
  tcp::socket socket_;
  enum { max_length = 1024 };
  char data_[max_length];
};

class server
{
public:
  server(boost::asio::io_context& context, short port)
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
          if (!ec)
            std::make_shared<session>(std::move(socket))->start();
          do_accept();
        });
  }
  tcp::acceptor acceptor_;
};
string env_query(string s){
    char *query = getenv(s.c_str());
    if (query==nullptr)
        return "None";
    return query;
}
int main(int argc, char* argv[])
{
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