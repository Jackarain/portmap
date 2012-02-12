#include "stdafx.h"
#include "logger.hpp"
#include "netmap.hpp"


struct configure
{
public:
   std::string remote_host;
   std::string dump_file;  // 未实现的功能.
   int server_port;
   int is_auto_connect;    // 未实现的功能.
   int time_out;           // 未实现的功能.

public:
   void load_config_file() 
   {
      boost::program_options::options_description config("Configuration");
      config.add_options()
         ("auto connect", boost::program_options::value<int>(&is_auto_connect)->default_value(0), "断开自动重连, 未实现.")
         ("time out", boost::program_options::value<int>(&time_out)->default_value(0), "超时时间, 未实现.")
         ("server port", boost::program_options::value<int>(&server_port)->default_value(4322), "本地服务端口, 默认4322.")
         ("remote host", boost::program_options::value<std::string>(&remote_host)->default_value("127.0.0.1:2080"), "远程主机的host:port.")
         ;

      std::ifstream ifs("conf.cfg");
      boost::program_options::variables_map vm;
      boost::program_options::options_description config_file_options;

      config_file_options.add(config);
      store(parse_config_file(ifs, config_file_options), vm);
      notify(vm);

      log_info("auto connect: \t" << is_auto_connect << "\n");
      log_info("time out: \t" << time_out << " milliseconds." << "\n");
      log_info("remote host: \t\t" << remote_host.c_str() << "\n");
      log_info("local server port: \t" << server_port << "\n\n");
   }
};

int main(int argc, char* argv[])
{
   try 
   {
      configure cfg;
      std::string host;
      std::string port;
      std::string url;
      boost::asio::io_service io_service;

      boost::regex expression("^([^:]*)[:]([^//]*)(.*)$");

      // 加载配置文件.
      cfg.load_config_file();

      // 得到源url.
      url = cfg.remote_host;

      std::string::const_iterator start, end;

      start = cfg.remote_host.begin();
      end = cfg.remote_host.end();
      boost::match_results<std::string::const_iterator> what; 
      boost::match_flag_type flags = boost::match_default; 
      if (regex_search(start, end, what, expression, flags)) {
         host = std::string(what[1]);
         port = std::string(what[2]);
      }

      if (host.empty() || port.empty()) {
         std::cout << "错误: 必须设置远程主机ip和port!\n";
         return -1;
      }

      tcp::endpoint ep(boost::asio::ip::address::from_string(host), 
         atoi(port.c_str()));

      // 一个转发服务器.
      netmap_server server(io_service, cfg.server_port, ep, cfg.dump_file);

      io_service.run();
   }
   catch (std::exception& e)
   {
      std::cout << "Exception: " << e.what() << "\n";
   }
   catch (...)
   {
      return -1;
   }

   return 0;
}

