#include <stdio.h>
#include <future>
#include "V4l2Capture.h"
#include "logger.h"

static int readOne(const char* name, V4l2Capture* capture, int M) {
  int n = 0;
  printf("%s: started\n", name );
  for (int i = 0; i < M; i++) {
    timeval timeout;
    timeout.tv_usec = 0;
    timeout.tv_sec = 1;
    bool isReadable = (capture->isReadable(&timeout) == 1);
    if (isReadable) {
      char buffer[3 * 640 * 480];
      /*int nb =*/ capture->read(buffer, sizeof buffer);
      //printf("%s: %d read\n", name, nb);
      n++;
    } else {
      //printf("%s: not read\n", name);
    }
  }
  printf("%s: stopped\n", name );
  return n;
}

bool waitReady( const char* path ) {
  for (int i = 0; i < 10; i++) {
    FILE* f = fopen(path, "rb");
    if (f != nullptr) {
      fclose(f);
      return true;
    }
    sleep(1);
  }
  return false;
}

int main() {
#define V4L2_PIX_FMT_MJPG v4l2_fourcc('M', 'J', 'P', 'G')
#define N 7
#define VERBOSE 0

  const char* devices[N] = {
      "/dev/video0", "/dev/video2",  "/dev/video4",  "/dev/video6",
      "/dev/video8", "/dev/video10", "/dev/video12",
  };

  V4l2Capture* captures[N];

  std::future<int> threads[N];

  initLogger(VERBOSE);

  for (int i = 0; i < N; i++) {
    if (! waitReady(devices[i])) {
      printf("%s inaccessible\n", devices[i]);
      continue;
    }
    V4L2DeviceParameters param(devices[i], V4L2_PIX_FMT_MJPG, 640, 480, 10,
                               IOTYPE_MMAP, VERBOSE);
    captures[i] = V4l2Capture::create(param);
    //int n = readOne( devices[i], captures[i], 10 );
    //printf("%s: read unthreaded %d\n", devices[i], n);
  }

  for (int i = 0; i < N; i++) {
    if (captures[i] == nullptr)
      continue;
    threads[i] = std::async(std::launch::async, readOne, devices[i], captures[i], 100 );
  }
  for (int i = 0; i < N; i++) {
    if (captures[i] == nullptr)
      continue;
    int n = threads[i].get();
    printf("%s: read threaded %d\n", devices[i], n);
  }
}
