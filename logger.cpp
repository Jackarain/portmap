#include "stdafx.h"
#include "logger.hpp"

namespace portmap_logger {

#ifndef LOGGING_NUMBER
#define LOGGING_NUMBER 0
#endif // LOGGING_NUMBER

#ifdef WIN32

#include <stdarg.h>

// '_vsnprintf': This function or variable may be unsafe
#pragma warning(disable:4996)

inline int snprintf(char* buf, int len, char const* fmt, ...)
{
   va_list lp;
   va_start(lp, fmt);
   int ret = _vsnprintf(buf, len, fmt, lp);
   va_end(lp);
   if (ret < 0) { buf[len-1] = 0; ret = len-1; }
   return ret;
}

#endif // WIN32

char const* time_now_string()
{
   time_t t = std::time(0);
   tm* timeinfo = std::localtime(&t);
   static char str[200];
   std::strftime(str, 200, " %b %d %X ", timeinfo);
   return str;
}

inline ptime time_now_hires()
{ return boost::posix_time::microsec_clock::universal_time(); }

namespace aux {
   ptime g_current_time = time_now_hires();
   boost::mutex g_logger_mutex;
   boost::shared_ptr<logger> g_logger;
}

ptime const& time_now() { return aux::g_current_time; }

inline int total_milliseconds(time_duration td)
{ return td.total_milliseconds(); }

std::string log_time()
{
   static const ptime start = time_now_hires();
   char ret[200];
   std::sprintf(ret, "%d", total_milliseconds(time_now() - start));
   return ret;
}

boost::shared_ptr<logger> logger_() {
   if (!aux::g_logger) {
      aux::g_logger.reset(new logger(".", "portmap.log", LOGGING_NUMBER));
   }
   return aux::g_logger;
}

} // namespace portmap_logger
