#ifndef __NETMAPSERVER_H__
#define __NETMAPSERVER_H__

#pragma once

class handler_allocator
   : private boost::noncopyable
{
public:
   handler_allocator()
      : in_use_(false)
   {
   }

   void* allocate(std::size_t size)
   {
      if (!in_use_ && size < storage_.size)
      {
         in_use_ = true;
         return storage_.address();
      }
      else
      {
         return ::operator new(size);
      }
   }

   void deallocate(void* pointer)
   {
      if (pointer == storage_.address())
      {
         in_use_ = false;
      }
      else
      {
         ::operator delete(pointer);
      }
   }

private:
   // Storage space used for handler-based custom memory allocation.
   boost::aligned_storage<1024> storage_;

   // Whether the handler-based custom allocation storage has been used.
   bool in_use_;
};

template <typename Handler>
class custom_alloc_handler
{
public:
   custom_alloc_handler(handler_allocator& a, Handler h)
      : allocator_(a),
      handler_(h)
   {
   }

   template <typename Arg1>
   void operator()(Arg1 arg1)
   {
      handler_(arg1);
   }

   template <typename Arg1, typename Arg2>
   void operator()(Arg1 arg1, Arg2 arg2)
   {
      handler_(arg1, arg2);
   }

   friend void* asio_handler_allocate(std::size_t size,
      custom_alloc_handler<Handler>* this_handler)
   {
      return this_handler->allocator_.allocate(size);
   }

   friend void asio_handler_deallocate(void* pointer, std::size_t /*size*/,
      custom_alloc_handler<Handler>* this_handler)
   {
      this_handler->allocator_.deallocate(pointer);
   }

private:
   handler_allocator& allocator_;
   Handler handler_;
};

// Helper function to wrap a handler object to add custom allocation.
template <typename Handler>
inline custom_alloc_handler<Handler> make_custom_alloc_handler(
   handler_allocator& a, Handler h)
{
   return custom_alloc_handler<Handler>(a, h);
}


class portmap_session
   : public boost::enable_shared_from_this<portmap_session>
{
public:
   portmap_session(boost::asio::io_service& io_service, tcp::endpoint& ep);
   ~portmap_session();

   //////////////////////////////////////////////////////////////////////////
   tcp::socket& socket() { return m_local_socket; }

   void start();

   void close();

   void handle_local_read(const boost::system::error_code& err, 
      int bytes_transferred);

   void handle_local_write(boost::shared_ptr<std::vector<char> > buffer, 
      size_t bytes_transferred, const boost::system::error_code& error);

   void local_write(const char* buffer, int buffer_length);

   //////////////////////////////////////////////////////////////////////////
   void remote_connect(const boost::system::error_code& err);

   void handle_remote_read(const boost::system::error_code& err, 
      int bytes_transferred);

   void handle_remote_write(boost::shared_ptr<std::vector<char> > buffer, 
      size_t bytes_transferred, const boost::system::error_code& error);

   void remote_write(const char* buffer, int buffer_length);

private:
   tcp::socket m_local_socket;
   boost::array<char, 1024> m_local_buffer;
   handler_allocator m_allocator;
   tcp::endpoint m_local_host;

   tcp::endpoint m_remote_host;
   tcp::socket m_remote_socket;
   boost::array<char, 1024> m_remote_buffer;

   static int m_user_cnt;
   static boost::mutex m_user_mtx;
};

typedef boost::shared_ptr<portmap_session> portmap_session_ptr;


class netmap_server
   : public boost::noncopyable
{
public:
   netmap_server(boost::asio::io_service& io_service, 
      short server_port, tcp::endpoint& ep, const std::string& dump_file);
   ~netmap_server(void) {}

   void handle_accept(portmap_session_ptr new_session,
      const boost::system::error_code& error);

private:
   boost::asio::io_service& m_io_service;
   tcp::acceptor m_acceptor;
   tcp::endpoint m_remote_host;
   boost::thread m_thread;
   mutable boost::mutex m_mutex;
};

#endif // __NETMAPSERVER_H__

