#include "Timer.h"

#include <sys/timerfd.h>
#include <sys/epoll.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

namespace common 
{

Timer::Timer(int n):
  epfd_(epoll_create(256)),
  timerfd_(timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK|TFD_CLOEXEC)),
  cur_(0),
  total_(n),
  quit_(0),
  timer_list_(total_, nullptr)
{
  for (int i=0; i < total_; ++i) {
    timer_list_[i] = new TimerInfo_t();
  }
}

Timer::~Timer()
{
  ::close(epfd_);
  ::close(timerfd_);
  for (int i=0; i < total_; ++i) {
    delete timer_list_[i];
  }
}

int Timer::Add(int ms, TIMER_CALLBACK_t cb)
{
  TimerNode_t* node = new TimerNode_t();
  node->cycle = ms / total_;
  node->cb = std::move(cb);
  node->ms = ms;

  int slot = ms % total_ + cur_;
  slot %=  total_;

  TimerInfo_t* info = timer_list_[slot];
  if (info) {
    info->Add(node);
    return 0;
  }
  return -1;
}

int Timer::Run()
{
  int n;
  struct epoll_event events[1];
  while (!quit_) {
    n = ::epoll_wait(epfd_, events, 1, 1);
    if (n==0) { //timeout
      TimerInfo_t* timeo_list = timer_list_[cur_];
      adjust(timeo_list);
      cur_ = (++cur_) % total_;
    } else if (errno == EINTR) {
      continue;
    } else {
      quit_ = 1;
    }
  }
  return 0;
}

int Timer::adjust(TimerInfo_t* tlist)
{
  if (nullptr == tlist) {
    return 0;
  }
  TimerNode_t* head = &tlist->head;
  TimerNode_t* tail = &tlist->tail;
  TimerNode_t* node = head->next;
  TimerNode_t* next = node;
  while (node != tail) {
    next = node->next;
    if (node->cycle == 0) {
      node->cb();
      tlist->Delete(node);
      delete node;
    } else {
      node->cycle--;
    }
    node = next;
  }
  return tlist->Empty();
}

void Timer::Cancel()
{
  quit_ = 1;
}

}

