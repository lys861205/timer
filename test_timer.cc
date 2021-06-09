#include "Timer.h"

#include <unistd.h>
#include <time.h>

using namespace common;

void func(std::string& str)
{
  printf("timeout %s\n", str.c_str());
}

void timer_cb(int& a, uint64_t start_time)
{
  struct timespec end;
  clock_gettime(CLOCK_MONOTONIC, &end);
  uint64_t end_time = end.tv_sec * 1000000000ULL + end.tv_nsec;
  printf("timer_cb %ld\n", (end_time - start_time)/1000000);
  a = 0;
}

void* thread_func(void* p)
{
  Timer* ptr = (Timer*)p;
  int a = 1;
  struct timespec start;
  clock_gettime(CLOCK_MONOTONIC, &start);
  uint64_t start_time = start.tv_sec * 1000000000ULL + start.tv_nsec;
  ptr->Add(20000, std::bind(timer_cb, std::ref(a), start_time));
  while (a);
  ptr->Cancel();
}

int main()
{
  Timer t;
  pthread_t tid;
  pthread_create(&tid, NULL, thread_func, &t);
  std::string str("hello 20");
  t.Add(20, std::bind(func, str));
  str = "hello 22";
  t.Add(22, std::bind(func, str));
  str = "hello 10000";
  t.Add(10000, std::bind(func, str));
  t.Run();
  pthread_join(tid, NULL);
  return 0;
}
