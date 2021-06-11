#ifndef __HEAP_H__
#define __HEAP_H__

#include <stdlib.h>
#include <assert.h>

#include <algorithm>
#include <functional>

template <typename T, 
          typename Compare = std::less<T>
          >
class Heap 
{
public:
  Heap(int n=1024, const Compare& cmp = Compare()):
    cur_(0), 
    size_(n),
    cmp_(cmp)
  {
    elems_ = (T*)malloc(sizeof(T)*(n+1));
    assert(elems_);
  }

  ~Heap() 
  {
    if (elems_)  {
      free(elems_);
    }
  }

  int insert(T&& value)
  {
    if (cur_ >= size_) {
      //扩容
      resize();
    }
    ++cur_;
    elems_[cur_] = value;
    // 上浮
    swin();
    return 0;
  }

  int insert(const T& value)
  {
    if (cur_ >= size_) {
      //扩容
      resize();
    }
    ++cur_;
    elems_[cur_] = value;
    // 上浮
    swin();
    return 0;
  }
  bool empty() const 
  {
    return cur_ > 0 ? false : true;
  }
  T& best() 
  {
    return cur_ > 0 ? elems_[1] : elems_[0];
  }
  const T& best() const 
  {
    return cur_ > 0 ? elems_[1] : elems_[0];
  }
  int pop()
  {
    if (empty()) {
      return -1;
    }
    std::swap(elems_[cur_], elems_[1]);
    cur_--; 
    sink();
    return 0;
  }
  int size() const 
  {
    return cur_;
  }
private:

  void swin()
  {
    int N = cur_; 
    int p = N/2;
    while (N > 1 && cmp_(elems_[N], elems_[p])) {
      std::swap(elems_[p], elems_[N]);
      N = p;
      p = N/2;
    }
  }

  void sink()
  {
    int N = cur_;
    int pos = 1;
    int l = 2*pos;
    while (l <= N) {
      int r = 2*pos + 1;
      if (r <= N && cmp_(elems_[r], elems_[l])) {
        l = r;
      }
      if (cmp_(elems_[pos], elems_[l])) {
        break;
      }
      std::swap(elems_[l], elems_[pos]);
      pos = l;
      l = 2 * pos;
    }
  }

  int resize()
  {
    size_ *= 2;
    elems_ = (T*)realloc(elems_, sizeof(T) * (size_ + 1));
    if (elems_) {
      return 0;
    }
    return -1;
  }

private:
  T* elems_;
  int cur_;
  int size_;
  Compare cmp_;
};

#endif// __HEAP_H__
