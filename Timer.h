#ifndef __TIMER_H__
#define __TIMER_H__

#include <vector>
#include <functional>

#include <pthread.h>

#ifdef __linux__
  #include <sys/timerfd.h>
  #include <sys/epoll.h>
#else  //MacOs
  #include <sys/types.h>
  #include <sys/event.h>
  #include <sys/time.h>
#endif 

#include "heap.h"

namespace common
{
using TIMER_CALLBACK_t = std::function<void(void)>;

typedef struct tag_TimerNode{
  struct timespec timeout;
  TIMER_CALLBACK_t cb;
} TimerNode_t;

struct CmpTimerNode {
  bool operator()(TimerNode_t* lh, TimerNode_t* rh) 
  {
    if (lh->timeout.tv_sec < rh->timeout.tv_sec) {
      return true; 
    }
    else if (lh->timeout.tv_sec == rh->timeout.tv_sec && 
             lh->timeout.tv_nsec <= rh->timeout.tv_nsec) {
      return true;
    }
    else {
      return false;
    }
  }
};


class Timer {
public:
  Timer(int n=100);
  ~Timer();
  int  Add(int ms, TIMER_CALLBACK_t cb);
  int  Run();
  void Cancel();
private:
  inline int SetTime(const struct timespec* abstime) 
  {
#ifdef __linux__
    struct itimerspec timer = {
      .it_interval  = { },
      .it_value   = *abstime
    };
    return timerfd_settime(timerfd_, TFD_TIMER_ABSTIME, &timer, NULL); 
#else  // MacOs
    struct timespec curtime;
    long long nseconds;
    struct kevent ev;
    if (abstime->tv_sec || abstime->tv_nsec)
    {
      clock_gettime(CLOCK_MONOTONIC, &curtime);
      nseconds = 1000000000LL * (abstime->tv_sec - curtime.tv_sec);
      nseconds += abstime->tv_nsec - curtime.tv_nsec;
      EV_SET(&ev, timerfd_, EVFILT_TIMER, EV_ADD, NOTE_NSECONDS, nseconds, NULL);
    } 
    return kevent(epfd_, &ev, 1, NULL, 0, NULL);
#endif //
  }
  inline int SetEvent()
  {
#ifdef __linux__
    struct epoll_event ev;
    ev.events = EPOLLIN;
    ev.data.fd = timerfd_;
    return ::epoll_ctl(epfd_, EPOLL_CTL_ADD, timerfd_, &ev);
#endif //
    return 0;
  }
  int SetMinTime();
  int HandleTimeout(const struct timespec* abstime);
  inline bool LessOrEqual(const struct timespec* l, const struct timespec* r)
  {
    if (l->tv_sec < r->tv_sec) {
      return true; 
    }
    else if (l->tv_sec == r->tv_sec && 
             l->tv_nsec <= r->tv_nsec) {
      return true;
    }
    return false;
  }
private:
  int epfd_;
  int timerfd_;
  int quit_;
  Heap<TimerNode_t*, CmpTimerNode> timer_list_;
  pthread_mutex_t mutex_;
};
}


#endif //__TIMER_H__
