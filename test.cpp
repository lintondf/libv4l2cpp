#include <stdio.h>
#include <string.h>
#include <chrono>
#include <future>
#include "V4l2Capture.h"
#include "logger.h"

using namespace std::chrono_literals;

Logger logger;

static int readOne(const char* name, V4l2Capture* capture, int M) {
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
    timeval timeout;
    timeout.tv_usec = 0;
    timeout.tv_sec = 1;
    bool isReadable = (capture->isReadable(&timeout) == 1);
    if (isReadable) {
      char buffer[3 * 640 * 480];
      int nb = capture->read(buffer, sizeof buffer);
      LOG4CPLUS_INFO_FMT(logger, "%s: %d/%d read", name, n, nb);
      n++;
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

  // initLogger(0);
  // Initialization and deinitialization.
  log4cplus::Initializer initializer;

  logger = log4cplus::Logger::getInstance(LOG4CPLUS_TEXT("main"));
  log4cplus::SharedAppenderPtr fileAppender(
      new log4cplus::FileAppender("/run/user/1000/test.log"));
  // log4cplus::Layout layout(new log4cplus::TTCCLayout());
  // fileAppender->setLayout(layout);
  logger.addAppender(fileAppender);
  log4cplus::LogLevel ll = DEBUG_LOG_LEVEL;
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
    parms[i] = new V4L2DeviceParameters(devices[i], V4L2_PIX_FMT_MJPG, 640, 480,
                                        (i ==0) ? 30 : 20, IOTYPE_MMAP, VERBOSE);
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
#if 1
  auto start = std::chrono::high_resolution_clock::now();
  threads[0] =
      std::async(std::launch::async, readOne, devices[0], captures[0], 1000);
  // for (int i = 0; i < 5; i++) {
  while (threads[0].wait_for(0s) != std::future_status::ready) {
    for (int j = 1; j < 4; j++) {
      threads[j] =
          std::async(std::launch::async, readOne, devices[j], captures[j], 3);
    }
    for (int j = 1; j < 4; j++) {
      counts[j]++;
      threads[j].get();
    }
    for (int j = 4; j < N; j++) {
      threads[j] =
          std::async(std::launch::async, readOne, devices[j], captures[j], 3);
    }
    for (int j = 4; j < N; j++) {
      counts[j]++;
      threads[j].get();
    }
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
  auto end = std::chrono::high_resolution_clock::now();
  std::chrono::duration<double> elapsed_seconds = end - start;
  for (int i = 0; i < N; i++) {
    printf("%d: %6.0f @ %6.3f fps; ", i, counts[i], counts[i] / elapsed_seconds.count() );
  }
  printf("\n");
  printf("%6.3f seconds\n", elapsed_seconds.count());
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