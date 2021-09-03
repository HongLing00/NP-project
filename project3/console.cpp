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
struct User 
  : public std::enable_shared_from_this<User> 
  {
    public:
    User(string session, string file_name, tcp::resolver::query que):
    session(std::move(session)), que(std::move(que)), timer(context)
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
                    /*timer.expires_from_now(boost::posix_time::seconds(1));
                    timer.async_wait([this, self](boost::system::error_code ec) {
                            write_handler();
                            read_handler() ;   
                    });*/
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
  deadline_timer timer;
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
    for(auto line : spilt_query(env_query("QUERY_STRING"))){
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
    cout << html_console();
    for (size_t i = 0; i < 5; i++) {     
      if (host[i].usage()) {
          boost::asio::ip::tcp::resolver::query que{host[i].hostname, host[i].port};
          make_shared<User>("s" + to_string(i), host[i].file, move(que))->start();
        }
    }
    context.run() ;
}