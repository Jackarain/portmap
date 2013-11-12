#include "stdafx.h"
#include "netmap.hpp"
#include "logger.hpp"

int portmap_session::m_user_cnt = 0;
boost::mutex portmap_session::m_user_mtx;

portmap_session::portmap_session(boost::asio::io_service& io_service, tcp::endpoint& ep)
   : m_local_socket(io_service)
   , m_remote_socket(io_service)
   , m_remote_host(ep)
{}

portmap_session::~portmap_session()
{
   boost::system::error_code ignored_ec;

   log_info("Id: " << m_user_cnt << ", IP: " << 
      m_local_host.address().to_string(ignored_ec).c_str() << " : " 
      << m_local_host.port() << " 离开.\n");

   {
      boost::mutex::scoped_lock lock(m_user_mtx);
      m_user_cnt--;
   }
}

void portmap_session::start()
{
   boost::system::error_code ignored_ec;

   {
      boost::mutex::scoped_lock lock(m_user_mtx);
      m_user_cnt++;
   }

   m_local_socket.set_option(tcp::no_delay(true), ignored_ec);

   m_local_host = m_local_socket.remote_endpoint(ignored_ec);
   log_info("Id: " << m_user_cnt << ", IP: "
      << m_local_host.address().to_string(ignored_ec).c_str() 
      << " : " << m_local_host.port() << " 进来.\n");

   // 发起远程连接.
   m_remote_socket.async_connect(m_remote_host, 
      make_custom_alloc_handler(m_allocator,
      boost::bind(&portmap_session::remote_connect, shared_from_this(),
      boost::asio::placeholders::error)));
}

void portmap_session::close()
{
   boost::system::error_code ignored_ec;

   // 远程和本地链接都将关闭.
   m_local_socket.shutdown(
      boost::asio::ip::tcp::socket::shutdown_both, ignored_ec);
   m_local_socket.close(ignored_ec);
   m_remote_socket.shutdown(
      boost::asio::ip::tcp::socket::shutdown_both, ignored_ec);
   m_remote_socket.close(ignored_ec);
}

void portmap_session::handle_local_read(const boost::system::error_code& error,
                                        int bytes_transferred)
{
   if (!error)
   {
      // 发送到remote.
      remote_write(m_local_buffer.data(), bytes_transferred);

      // 继续接收local数据.
      m_local_socket.async_read_some(boost::asio::buffer(m_local_buffer),
         make_custom_alloc_handler(m_allocator,
         boost::bind(&portmap_session::handle_local_read, shared_from_this(),
         boost::asio::placeholders::error,
         boost::asio::placeholders::bytes_transferred)));
   }
   else
   {
      close();
   }
}

void portmap_session::handle_local_write(boost::shared_ptr<std::vector<char> > buffer, 
                                         size_t bytes_transferred, const boost::system::error_code& error)
{
   if (!error)
   {
      // 继续发送未发送完成的数据.
      if (bytes_transferred != buffer->size())
      {
         local_write(&(*(buffer->begin() + bytes_transferred)), 
            buffer->size() - bytes_transferred);
      }
   }
   else
   {
      close();
   }
}

void portmap_session::local_write(const char* buffer, int buffer_length)
{
   boost::shared_ptr<std::vector<char> > buf(new 
      std::vector<char>(buffer, buffer + buffer_length));

   // 发送数据.
   m_local_socket.async_write_some(boost::asio::buffer(*buf),
      make_custom_alloc_handler(m_allocator, 
      boost::bind(&portmap_session::handle_local_write, 
      shared_from_this(),  buf,
      boost::asio::placeholders::bytes_transferred,
      boost::asio::placeholders::error)));
}

void portmap_session::remote_connect(const boost::system::error_code& err)
{
   if (!err)
   {
      boost::system::error_code ignored_ec;
      m_remote_socket.set_option(tcp::no_delay(true), ignored_ec);
      // 连接成功, 发起一个读请求.
      m_remote_socket.async_read_some(boost::asio::buffer(m_remote_buffer),
         make_custom_alloc_handler(m_allocator,
         boost::bind(&portmap_session::handle_remote_read, shared_from_this(),
         boost::asio::placeholders::error,
         boost::asio::placeholders::bytes_transferred)));
	  // 读取本地链接数据.
	  m_local_socket.async_read_some(boost::asio::buffer(m_local_buffer),
		  make_custom_alloc_handler(m_allocator,
		  boost::bind(&portmap_session::handle_local_read, shared_from_this(),
		  boost::asio::placeholders::error,
		  boost::asio::placeholders::bytes_transferred)));
   }
   else
   {
      close();
   }
}

void portmap_session::handle_remote_read(const boost::system::error_code& error,
                                         int bytes_transferred)
{
   if (!error)
   {
      // 发送到local.
      local_write(m_remote_buffer.data(), bytes_transferred);

      // 继续接收remote的数据.
      m_remote_socket.async_read_some(boost::asio::buffer(m_remote_buffer),
         make_custom_alloc_handler(m_allocator,
         boost::bind(&portmap_session::handle_remote_read, shared_from_this(),
         boost::asio::placeholders::error,
         boost::asio::placeholders::bytes_transferred)));
   }
   else
   {
      close();
   }
}

void portmap_session::handle_remote_write(boost::shared_ptr<std::vector<char> > buffer, 
                                          size_t bytes_transferred, const boost::system::error_code& error)
{
   if (!error)
   {
      // 继续发送未发送完成的数据.
      if (bytes_transferred != buffer->size())
      {
         remote_write(&(*(buffer->begin() + bytes_transferred)), 
            buffer->size() - bytes_transferred);
      }
   }
   else
   {
      close();
   }
}

void portmap_session::remote_write(const char* buffer, int buffer_length)
{
   boost::shared_ptr<std::vector<char> > buf(new 
      std::vector<char>(buffer, buffer + buffer_length));

   // 发送数据.
   m_remote_socket.async_write_some(boost::asio::buffer(*buf),
      make_custom_alloc_handler(m_allocator, 
      boost::bind(&portmap_session::handle_remote_write, 
      shared_from_this(),  buf,
      boost::asio::placeholders::bytes_transferred,
      boost::asio::placeholders::error)));
}



netmap_server::netmap_server(boost::asio::io_service& io_service, short server_port, 
                       tcp::endpoint& ep, const std::string& dump_file)
                       : m_io_service(io_service)
                       , m_acceptor(io_service, tcp::endpoint(tcp::v4(), server_port))
                       , m_remote_host(ep)
{
   portmap_session_ptr new_session(new portmap_session(m_io_service, m_remote_host));
   m_acceptor.async_accept(new_session->socket(),
      boost::bind(&netmap_server::handle_accept, this, new_session,
      boost::asio::placeholders::error));
}

void netmap_server::handle_accept(portmap_session_ptr new_session, 
                               const boost::system::error_code& error)
{
   if (!error)
   {
      new_session->start();
      new_session.reset(new portmap_session(m_io_service, m_remote_host));
      m_acceptor.async_accept(new_session->socket(),
         boost::bind(&netmap_server::handle_accept, this, new_session,
         boost::asio::placeholders::error));
   }
}

