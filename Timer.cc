#include "Timer.h"

#include <unistd.h>
#include <errno.h>
#include <string.h>

namespace common 
{

Timer::Timer(int n):
#ifdef __linux__
  epfd_(epoll_create(256)),
  timerfd_(timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK|TFD_CLOEXEC)),
#else 
  epfd_(kqueue()),
  timerfd_(dup(0)),
#endif
  quit_(0),
  timer_list_(n)
{
  if (pipe(pipefd_) != 0) {
    exit(-1);
  }
  SetPipe();
  mutex_ = PTHREAD_MUTEX_INITIALIZER;
  pthread_mutex_init(&mutex_, NULL);
}

Timer::~Timer()
{
  ::close(epfd_);
  ::close(timerfd_);
  ::close(pipefd_[0]);
  ::close(pipefd_[1]);
  pthread_mutex_destroy(&mutex_);
}

int Timer::Add(int ms, TIMER_CALLBACK_t cb)
{
  TimerNode_t* node = new TimerNode_t();
  node->cb = std::move(cb);
  clock_gettime(CLOCK_MONOTONIC, &node->timeout);
  node->timeout.tv_nsec += ms * 1000000ULL;
  node->timeout.tv_sec += node->timeout.tv_nsec / 1000000000ULL;
  node->timeout.tv_nsec %= 1000000000ULL;

  int ret = 0;
  TimerNode_t* last_node = nullptr;
  pthread_mutex_lock(&mutex_);
  if (!timer_list_.empty()) {
    last_node = timer_list_.best(); 
  }
  timer_list_.insert(node);
  if (!last_node || CmpTimerNode()(node, last_node)) {
    ret = SetTime(&node->timeout);
  }
  pthread_mutex_unlock(&mutex_);
  return ret;
}

int Timer::SetMinTime()
{
  if (timer_list_.empty()) {
    return -1;
  }
  pthread_mutex_lock(&mutex_);
  TimerNode_t* node = timer_list_.best();
  int ret = SetTime(&node->timeout);
  pthread_mutex_unlock(&mutex_);
  return ret;
}

int Timer::Run()
{
  AddTimer();
  int n;
  struct timespec abstime; 
  while (!quit_) {
    SetMinTime();
#ifdef __linux__ 
    struct epoll_event events[2];
    n = ::epoll_wait(epfd_, events, 2, -1);
#else 
    struct kevent events[2];
    n = kevent(epfd_, NULL, 0, events, 2, NULL); 
#endif 
    int fd;
    for (int i=0; i < n; ++i) {
#ifdef __linux__ 
      fd = events[i].data.fd; 
#else 
     if (events[i].udata) {
      fd = *(int*)events[i].udata;
     }
#endif 
      if (fd == pipefd_[0]) {
        printf("loop quit.\n");
        break;
      }
    }
    clock_gettime(CLOCK_MONOTONIC, &abstime);
    if (errno == EINTR) {
      continue;
    } else if (n < 0) {
      break;
    }
    HandleTimeout(&abstime);
  }
  return 0;
}

void Timer::Cancel()
{
  quit_ = 1;
  write(pipefd_[1], &quit_, sizeof(quit_));
}

int Timer::HandleTimeout(const struct timespec* abstime)
{
  TimerNode_t* node;
  pthread_mutex_lock(&mutex_);
  while (!timer_list_.empty()) {
    node = timer_list_.best(); 
    if (!LessOrEqual(&node->timeout, abstime)) {
      break;
    }
    node->cb();
    timer_list_.pop();
    delete node;
  }
  pthread_mutex_unlock(&mutex_);
  return 0;
}

}

