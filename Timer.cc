#include "Timer.h"

#include <unistd.h>
#include <errno.h>
#include <string.h>

namespace common 
{

Timer::Timer(int n):
  epfd_(epoll_create(256)),
  timerfd_(timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK|TFD_CLOEXEC)),
  quit_(0),
  timer_list_(n)
{
  mutex_ = PTHREAD_MUTEX_INITIALIZER;
  pthread_mutex_init(&mutex_, NULL);
}

Timer::~Timer()
{
  ::close(epfd_);
  ::close(timerfd_);
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
  TimerNode_t* node = timer_list_.best();
  return SetTime(&node->timeout);
}

int Timer::Run()
{
  SetEvent();
  int n;
  struct epoll_event events[1];
  struct timespec abstime; 
  while (!quit_) {
    SetMinTime();
    n = ::epoll_wait(epfd_, events, 1, -1);
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

