#ifndef __TIMTR_H__
#define __TIMER_H__

#include <vector>
#include <functional>

namespace common
{
using TIMER_CALLBACK_t = std::function<void(void)>;

typedef struct tag_TimerNode{
  uint64_t ms;
  int cycle;
  TIMER_CALLBACK_t cb;
  tag_TimerNode* next;
  tag_TimerNode* prev;
  tag_TimerNode():next(nullptr), prev(nullptr) {
  }
} TimerNode_t;

typedef struct tag_TimerInfo {
  TimerNode_t head;
  TimerNode_t tail;
  tag_TimerInfo() 
  {
    head.prev = nullptr;
    head.next = &tail;
    tail.prev = &head;
    tail.next = nullptr;
  }
  void Add(TimerNode_t* node)
  {
    node->next = head.next;
    head.next->prev = node;
    node->prev = &head;
    head.next = node;
  }
  void Delete(TimerNode_t* node) 
  {
    node->prev->next = node->next;
    node->next->prev = node->prev;
  }
  bool Empty() const {
    return head.next == &tail ? true : false;
  }
} TimerInfo_t;

class Timer {
public:
  Timer(int n=60);
  ~Timer();
  int  Add(int ms, TIMER_CALLBACK_t cb);
  int  Run();
  void Cancel();
private:
  int adjust(TimerInfo_t* tlist);
private:
  int epfd_;
  int timerfd_;
  int cur_;
  int total_;
  int quit_;
  std::vector<TimerInfo_t*> timer_list_;
};
}


#endif //__TIMER_H__
