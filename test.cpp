#include <stdio.h>
#include <string.h>
#include <chrono>
#include <future>
#include <string>
#include "RunningStats.h"
#include "V4l2Capture.h"
#include "logger.h"

/***
 *
 * For disabling HDMI audio, altering the config.txt line
"dtoverlay=vc4-kms-v3d" to "dtoverlay=vc4-kms-v3d,noaudio" disables it. The
analogue audio output can be disabled by omitting "dtparam=audio=on" from
config.txt.
*
 2685  sudo service --status-all
 2686  sudo service cups stop
 2687  sudo service --status-all
 2688  sudo service triggerhappy stop
 2689  sudo service bluetooth stop
 2690* sudo service --status-al
 2691  sudo service avahi-daemon stop


0:    300 @ 25.461 fps; 1:     24 @  2.037 fps; 2:     24 @  2.037 fps; 3: 24
@  2.037 fps; 4:     24 @  2.037 fps; 5:     24 @  2.037 fps; 6:     24 @  2.037
fps; 11.783 seconds

real    0m13.541s
user    0m0.123s
sys     0m1.044s

NO LOGGING
0:    996 @ 23.855 fps; 1:     74 @  1.772 fps; 2:     74 @  1.772 fps; 3: 74
@  1.772 fps; 4:     74 @  1.772 fps; 5:     74 @  1.772 fps; 6:     74 @  1.772
fps; 41.752 seconds

real    0m43.526s
user    0m0.063s
sys     0m2.217s

3 per
0:    994/22.655; 1:     74/ 1.687; 2:     74/ 1.687; 3:     74/ 1.687; 4:
74/ 1.687; 5:     74/ 1.687; 6:     74/ 1.687; 43.875 seconds

2 per
0:    998/25.168; 1:     84/ 2.118; 2:     84/ 2.118; 3:     84/ 2.118; 4:
84/ 2.118; 5:     84/ 2.118; 6:     84/ 2.118; 39.654 seconds



 */
using namespace std::chrono_literals;

Logger logger;

class Stats {
 public:
  Stats() { first = true; }

  double count() {
    auto next = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> diff;
    if (!first) {
      diff = next - last;
      running.Push(diff.count());
    }
    first = false;
    last = next;
    return diff.count();
  }

  std::string getState() {
    char buf[64];
    snprintf(buf, sizeof(buf), "%d: %5.3f (%5.3f) [%5.3f..%5.3f]",
             (int)running.NumDataValues(), running.Mean(),
             running.StandardDeviation(), running.Min(), running.Max());
    return std::string(buf);
  }

  bool first;
  RunningStats running;
  std::chrono::_V2::system_clock::time_point last;
};

static int delays = 0;

static int readOne(const char* name,
                   V4l2Capture* capture,
                   int M,
                   Stats* stats) {
  int n = 0;
  capture->start();
  LOG4CPLUS_INFO_FMT(logger, "%s: started", name);
  for (int i = 0; i < M; i++) {
#if 0
    char buffer[3 * 640 * 480];
    int nb = 0;
    do {
      usleep(1000);
      nb = capture->read(buffer, sizeof buffer);
    } while (nb <= 0);
    // printf("%s: %d read\n", name, nb);
    n++;
    if ((n % 1) == 0)
      printf("%s %-15s: %3d %5d\n", getCurrentTimestamp().c_str(), name, n, nb);
#else
    int nb = 0;
    char buffer[3 * 640 * 480];
    timeval timeout;
    timeout.tv_usec = 0;
    timeout.tv_sec = 1;
    bool isReadable = (capture->isReadable(&timeout) == 1);
    if (isReadable) {
      nb = capture->read(buffer, sizeof buffer);
      LOG4CPLUS_INFO_FMT(logger, "%s: read %d %d", name, n, nb);
      double dt = stats->count();
      if (dt > 0.5) {
        LOG4CPLUS_WARN_FMT(logger, "DELAY %s %6.3f\n", name, dt);
        if (strcmp(name, "/dev/video0") == 0) {
          printf("%s video0 delayed %6.3f\n", getCurrentTimestamp().c_str(),
                 dt);
          delays++;
        }
      }
      n++;
      // isReadable = (capture->isReadable(&timeout) == 1);
    }
    if (nb > 0) {
      // do something with buffer
    }
#endif
  }
  capture->stop();
  LOG4CPLUS_INFO_FMT(logger, "%s: stopped %d", name, n);
  return n;
}

bool waitReady(const char* path) {
  for (int i = 0; i < 10; i++) {
    FILE* f = fopen(path, "rb");
    if (f != nullptr) {
      fclose(f);
      return true;
    }
    sleep(1);
  }
  char cmd[128];
  sprintf(cmd, "lsof %s", path);
  std::cout << cmd << std::endl;
  system(cmd);
  return false;
}

#define K0 1000
#define Kn 0

void allThreaded(const char* devices[],
                 V4l2Capture* captures[],
                 std::future<int> threads[],
                 Stats stats[],
                 double counts[]) {
  const int N = 7;
  
  threads[0] = std::async(std::launch::async, readOne, devices[0], captures[0],
                          K0, &stats[0]);
  // for (int i = 0; i < 5; i++) {
  while (threads[0].wait_for(0s) != std::future_status::ready) {
    for (int j = 1; j < 4; j++) {
      threads[j] = std::async(std::launch::async, readOne, devices[j],
                              captures[j], Kn, &stats[j]);
    }
    for (int j = 1; j < 4; j++) {
      counts[j]++;
      threads[j].get();
    }
    for (int j = 4; j < N; j++) {
      threads[j] = std::async(std::launch::async, readOne, devices[j],
                              captures[j], Kn, &stats[j]);
    }
    for (int j = 4; j < N; j++) {
      counts[j]++;
      threads[j].get();
    }
    usleep(100);
  }
  if (threads[0].valid())
    counts[0] = threads[0].get();
}

int main() {
#define V4L2_PIX_FMT_MJPG v4l2_fourcc('M', 'J', 'P', 'G')
#define VERBOSE 0

  const char* devices[] = {
      "/dev/video0", "/dev/video2",  "/dev/video4",  "/dev/video6",
      "/dev/video8", "/dev/video10", "/dev/video12",
  };

#define N 7  //((int)(sizeof(devices) / sizeof(devices[0])))
  V4L2DeviceParameters* parms[N];
  V4l2Capture* captures[N];

  std::future<int> threads[N];

  double counts[N];

  Stats stats[N];

  // initLogger(0);
  // Initialization and deinitialization.
  log4cplus::Initializer initializer;

  logger = log4cplus::Logger::getInstance(LOG4CPLUS_TEXT("main"));
  log4cplus::SharedAppenderPtr fileAppender(
      new log4cplus::FileAppender("/run/user/1000/test.log"));
  // log4cplus::Layout layout(new log4cplus::TTCCLayout());
  // fileAppender->setLayout(layout);
  log4cplus::tstring pattern = LOG4CPLUS_TEXT(
      "%d{%m/%d/%y %H:%M:%S:%Q} [%t] %-6c %-5p %m%n");  // ,%Q [%l] %c{2} %%%x%%
                                                        // - %X
  //	std::tstring pattern = LOG4CPLUS_TEXT("%d{%c} [%t] %-5p [%.15c{3}]
  //%%%x%% - %m [%l]%n");
  fileAppender->setLayout(std::unique_ptr<Layout>(new PatternLayout(pattern)));

  logger.addAppender(fileAppender);
  log4cplus::LogLevel ll = INFO_LOG_LEVEL;
  logger.setLogLevel(ll);
  LOG4CPLUS_INFO_FMT(logger, "==============================================");

  int M = 0;

  for (int i = 0; i < N; i++) {
    counts[i] = 0;
    LOG4CPLUS_INFO_FMT(logger, "checking %s", devices[i]);
    if (!waitReady(devices[i])) {
      printf("%s inaccessible\n", devices[i]);
      parms[i] = nullptr;
      return 1;
    }
    parms[i] =
        new V4L2DeviceParameters(devices[i], V4L2_PIX_FMT_MJPG, 640, 480,
                                 (i == 0) ? 30 : 20, IOTYPE_MMAP, VERBOSE);
  }

  // int service[N] = {3, 6, 0, 2, 5, 1, 4};
  for (int i = M; i < N; i++) {
    int j = i;  // service[i];
    if (parms[j] == nullptr)
      continue;
    auto start = std::chrono::high_resolution_clock::now();

    captures[j] = V4l2Capture::create(*parms[j], false);

    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed_seconds = end - start;
    // std::cout << "\nelapsed time: " << elapsed_seconds.count() << "s\n";
    LOG4CPLUS_INFO_FMT(logger, "opened %s: fd=%d; %10.6f seconds", devices[j],
                       captures[j]->getFd(), elapsed_seconds.count());
  }
/*#if 1*/
  auto start = std::chrono::high_resolution_clock::now();
  allThreaded(devices, captures, threads, stats, counts);
/*  
  threads[0] = std::async(std::launch::async, readOne, devices[0], captures[0],
                          10000, &stats[0]);
  // for (int i = 0; i < 5; i++) {
  while (threads[0].wait_for(0s) != std::future_status::ready) {
    for (int j = 1; j < 4; j++) {
      threads[j] = std::async(std::launch::async, readOne, devices[j],
                              captures[j], 0, &stats[j]);
    }
    for (int j = 1; j < 4; j++) {
      counts[j]++;
      threads[j].get();
    }
    for (int j = 4; j < N; j++) {
      threads[j] = std::async(std::launch::async, readOne, devices[j],
                              captures[j], 0, &stats[j]);
    }
    for (int j = 4; j < N; j++) {
      counts[j]++;
      threads[j].get();
    }
    usleep(100);
  }
  if (threads[0].valid())
    counts[0] = threads[0].get();
#else
  for (int i = M; i < N; i++) {
    int j = i;  // service[i];
    if (parms[j] == nullptr)
      continue;
    threads[j] =
        std::async(std::launch::async, readOne, devices[j], captures[j], 25);
  }
  for (int i = 0; i < N; i++) {
    if (!threads[i].valid())
      continue;
    int n = threads[i].get();
    printf("%s: read threaded %d; v %d\n", devices[i], n, threads[i].valid());
  }
#endif
*/
  auto end = std::chrono::high_resolution_clock::now();
  std::chrono::duration<double> elapsed_seconds = end - start;
  for (int i = 0; i < N; i++) {
    std::cout << stats[i].getState();
    // printf("%d: %6.0f/%6.3f; ", i, counts[i],
    //       counts[i] / elapsed_seconds.count());
  }
  std::cout << std::endl;
  printf("%6.3f seconds\n", elapsed_seconds.count());
  printf("%d delays on video0\n", delays);
  return 0;
}

int mainbad() {
#define V4L2_PIX_FMT_MJPG v4l2_fourcc('M', 'J', 'P', 'G')

  V4L2DeviceParameters* parm = new V4L2DeviceParameters(
      "/dev/video2", V4L2_PIX_FMT_MJPG, 640, 480, 30, IOTYPE_READWRITE, 1);
  V4l2Capture* capture = V4l2Capture::create(*parm);
  for (int i = 0; i < 10; i++) {
    timeval timeout;
    timeout.tv_usec = 0;
    timeout.tv_sec = 1;
    bool isReadable = (capture->isReadable(&timeout) == 1);
    if (isReadable) {
      char buffer[3 * 640 * 480];
      int nb = capture->read(buffer, sizeof buffer);
      printf("%d: %d read\n", i, nb);
    } else {
      printf("%d: not ready\n", i);
    }
  }
  return 0;
}