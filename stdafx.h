#ifndef __STDAFX_H__
#define __STDAFX_H__

#pragma once

#ifdef WIN32
#define WIN32_LEAN_AND_MEAN		// 从 Windows 头中排除极少使用的资料
#endif

#include <stdio.h>

#include <iostream>
#include <istream>
#include <ostream>
#include <vector>
#include <string>

#include <boost/aligned_storage.hpp>
#include <boost/array.hpp>
#include <boost/thread.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/condition.hpp>
#include <boost/date_time.hpp>
#include <boost/noncopyable.hpp>
#include <boost/circular_buffer.hpp>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/program_options.hpp>
#include <boost/regex.hpp>
#define BOOST_FILESYSTEM_VERSION 2
#include <boost/filesystem.hpp>

using boost::asio::ip::tcp;

#endif // __STDAFX_H__

