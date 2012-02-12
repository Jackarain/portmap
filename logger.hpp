//
// http_logger.hpp
// ~~~~~~~~~~~~~~~
//
// Copyright (c) 2011 Jack (jack.wgm@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// * $Id: http_logger.hpp 67 2011-07-19 08:21:07Z jack $
//

#ifndef __LOGGER_H__
#define __LOGGER_H__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
# pragma once
#endif

#ifdef _MSC_VER
#pragma warning(push, 1)
#endif

#include <iostream>
#include <boost/filesystem/fstream.hpp>
#include <boost/filesystem/convenience.hpp>
#include <boost/date_time/posix_time/posix_time_types.hpp>

#ifdef _MSC_VER
#pragma warning(pop)
#endif

namespace portmap_logger {

namespace fs = boost::filesystem;
typedef boost::posix_time::ptime ptime;
typedef boost::posix_time::time_duration time_duration;

#ifdef WIN32
inline int snprintf(char* buf, int len, char const* fmt, ...);
#endif // WIN32

char const* time_now_string();
inline ptime time_now_hires();
ptime const& time_now();
inline int total_milliseconds(time_duration td);
std::string log_time();


class logger
{
public:
   logger(fs::path const& logpath, fs::path const& filename, int instance, bool append = true)
   {
      try
      {
         char log_name[1024];
         snprintf(log_name, sizeof(log_name), "logs%d", instance);
         fs::path dir(fs::complete(logpath / log_name));
         if (!fs::exists(dir)) fs::create_directories(dir);
         m_file.open((dir / filename).string().c_str(), std::ios_base::out | (append ? std::ios_base::app : std::ios_base::out));
         *this << "\n\n\n*** starting log ***\n\n\n";
      }
      catch (std::exception& e)
      {
         std::cerr << "failed to create log '" << filename.string() << "': " << e.what() << std::endl;
      }
   }

   template <class T>
   logger& operator<<(T const& v)
   {
      std::cout << v;
      std::cout.flush();

      m_file << v;
      m_file.flush();

      std::ostringstream oss;
      oss << v;
      m_last_log += oss.str();

      return *this;
   }

   std::string last_log() {
      return m_last_log;
   }

   void clear_last_log() {
      m_last_log = "";
   }

private:
   std::string m_last_log;
   std::ofstream m_file;
};

namespace aux {
extern ptime g_current_time;
extern boost::mutex g_logger_mutex;
extern boost::shared_ptr<logger> g_logger;
}


boost::shared_ptr<logger> logger_();

#define logs (*portmap_logger::logger_())

#ifdef LOGGER_THREAD_SAFE
#define locks() boost::mutex::scoped_lock lock(aux::g_logger_mutex)
#else
#define locks() do {} while (0)
#endif // LOGGER_THREAD_SAFE

#if defined(WIN32) && defined(LOGGER_DEBUG_VIEW)
#define debug_view() do { OutputDebugStringA(logs.last_log().c_str()); logs.clear_last_log(); } while (0)
#else
#define debug_view() do { } while (0)
#endif // WIN32 && LOGGER_DEBUG_VIEW

#ifdef LOGGER_OUTPUT_LOG

#define log_debug(message) do { \
   locks(); \
   logs << time_now_string() << "[DEBUG]: " << message; \
   debug_view(); \
} while (0)

#define log_info(message) do { \
   locks(); \
   logs << time_now_string() << "[INFO]: " << message; \
   debug_view(); \
} while (0)

#define log_warning(message) do { \
   locks(); \
   logs << time_now_string() << "[WARNING]: " << message; \
   debug_view(); \
} while (0)

#define log_error(message) do { \
   locks(); \
   logs << time_now_string() << "[ERROR]: " << message; \
   debug_view(); \
} while (0)

#else

#define log_debug(message) do { } while (0)
#define log_info(message) do { } while (0)
#define log_warning(message) do { } while (0)
#define log_error(message) do { } while (0)

#endif // LOGGER_OUTPUT_LOG

} // namespace portmap_logger

using namespace portmap_logger;

#endif // __LOGGER_H__

