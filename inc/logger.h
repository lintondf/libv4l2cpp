/* ---------------------------------------------------------------------------
** This software is in the public domain, furnished "as is", without technical
** support, and with no warranty, express or implied, as to its usefulness for
** any purpose.
**
** logger.h
**
** -------------------------------------------------------------------------*/

#ifndef LOGGER_H
#define LOGGER_H

#include <unistd.h>

#if 0
#include "log4cpp/Category.hh"
#include "log4cpp/FileAppender.hh"
#include "log4cpp/PatternLayout.hh"

#define LOG(__level)           \
  log4cpp::Category::getRoot() \
      << log4cpp::Priority::__level << __FILE__ << ":" << __LINE__ << "\n\t"

inline void initLogger(int verbose) {
  // initialize log4cpp
  log4cpp::Category& log = log4cpp::Category::getRoot();
  log4cpp::Appender* app = new log4cpp::FileAppender("root", fileno(stdout));
  if (app) {
    log4cpp::PatternLayout* plt = new log4cpp::PatternLayout();
    if (plt) {
      plt->setConversionPattern("%d [%-6p] - %m%n");
      app->setLayout(plt);
    }
    log.addAppender(app);
  }
  switch (verbose) {
    case 2:
      log.setPriority(log4cpp::Priority::DEBUG);
      break;
    case 1:
      log.setPriority(log4cpp::Priority::INFO);
      break;
    default:
      log.setPriority(log4cpp::Priority::ERROR);
      break;
  }
  LOG(INFO) << "level:"
            << log4cpp::Priority::getPriorityName(log.getPriority());
}
#else

typedef enum {
  EMERG = 0,
  FATAL = 0,
  ALERT = 100,
  CRIT = 200,
  ERROR = 300,
  WARN = 400,
  NOTICE = 500,
  INFO = 600,
  DEBUG = 700,
  NOTSET = 800
} PriorityLevel;

#include <string.h>
#include <chrono>
#include <fstream>
#include <iostream>
#if 0
extern std::ofstream logstream;
inline void initLogger(int verbose) {
	logstream.setstate(std::ios_base::badbit);
}
#define LOG(__level) \
  if (0)             \
  logstream

#else
#include <log4cplus/fileappender.h>
#include <log4cplus/helpers/loglog.h>
#include <log4cplus/helpers/property.h>
#include <log4cplus/initializer.h>
#include <log4cplus/configurator.h>
#include <log4cplus/layout.h>
#include <log4cplus/logger.h>
#include <log4cplus/loggingmacros.h>
#include <log4cplus/ndc.h>
#include <string>
#include <system_error>

using namespace log4cplus;

extern Logger logger;

inline void LOG4CPLUS_PERROR(Logger logger, const char* message) {
  LOG4CPLUS_ERROR(logger, LOG4CPLUS_TEXT(message) << " (" << errno << ") " << std::system_category().message(errno) );
}
// extern int LogLevel;
//#define LOG(__level)if (__level <= WARN) std::cout << "\n" <<
// getCurrentTimestamp() << " [" << #__level << "] " // << __FILE__ << ":" <<
//__LINE__ << "\n\t"

inline std::string getCurrentTimestamp() {
  using std::chrono::system_clock;
  auto currentTime = std::chrono::system_clock::now();
  char buffer[100];

  auto transformed = currentTime.time_since_epoch().count() / 1000000;

  auto millis = transformed % 1000;

  std::time_t tt;
  tt = system_clock::to_time_t(currentTime);
  auto timeinfo = localtime(&tt);
  strftime(buffer, 80, "%F %H:%M:%S", timeinfo);
  snprintf(buffer + strlen(buffer), sizeof(buffer) - strlen(buffer), ".%03d",
           (int)millis);

  return std::string(buffer);
}

inline void initLogger(int verbose) {
#if 1
#else
  switch (verbose) {
    case 2:
      LogLevel = DEBUG;
      break;
    case 1:
      LogLevel = INFO;
      break;
    default:
      LogLevel = WARN;
      break;
  }
  std::cout << "log level:" << LogLevel << std::endl;
#endif
}
#endif

#endif

#endif
