#ifndef MIN_HEAP
#define MIN_HEAP


#include <netinet/in.h>
#include <time.h>
#include <iostream>
using std::exception;

#define BUFFER_SIZE 64
class heap_timer; //前向声明

// 绑定 socket 和定时器
struct ClientData {
  sockaddr_in address;
  int sockfd;
  char buf[BUFFER_SIZE];
  heap_timer* timer;
};

// 定时器类
class heap_timer {
 public:
  heap_timer(int delay) {
    expire_ = time(NULL) + delay;
  }

 public:
  time_t expire_; //定时器生效的绝对时间
  void (*cb_func)(ClientData*); // 定时器的回调函数
  ClientData* user_data;
};

// 时间堆类
class time_heap {
 public:
 // 构造函数之一，初始化一个大小为cap的空堆
 time_heap(int cap) : capacity_(cap), cur_size_(0) {
  array_ = new heap_timer* [capacity_]; // 创建堆数组
  if (!array_) {
    throw std::exception();
  }

  for (int i = 0; i < capacity_; ++i) {
    array_[i] = NULL;
  }
}
  // 构造函数之二，用已有的数组来初始化堆
  time_heap(heap_timer** init_array, int size, int capacity) 
      : cur_size_(size), 
      capacity_(capacity) {
    if (capacity_ < size) {
      throw std::exception();
    }
    array_  = new heap_timer*[capacity_]; // 创建堆数组
    if (!array_) {
      throw std::exception();
    }
    for (int i = 0; i < capacity_; ++i) {
      array_[i] = NULL;
    }

    if (size != 0) {
      // 初始化堆数组
      for(int i = 0; i < size; ++i) {
        array_[i] = init_array[i];
      }

      // 对数组中的第[(cur_size - 1) / 2] ~ 0 个元素执行下沉操作 
      for (int i = (cur_size_) / 2; i >= 0; --i) {
        percolate_down(i);
      }
    }
 }

 ~time_heap() {
  for (int i = 0; i < cur_size_; ++i) {
    delete array_[i];
  }
  delete[] array_;
 }

 public:
  // 添加目标定时器timer
  void add_tiemr(heap_timer* timer) {
    if (!timer) {
      return;
    }

    // 如果当前堆数组的容量不够，则将其扩大一倍
    if (cur_size_ >= capacity_) {
      resize();
    }
  
    // 新插入了一个元素，当前堆大小加1，hole 是插入元素的位置
    int hole = cur_size_++;
    int parent = 0;
    // 堆新元素位置到根节点路径上的所有节点进行上升操作
    for (; hole > 0; hole = parent) {
      parent = (hole - 1) / 2;
      
      // 这个节点的父亲节点的超时时间比当前节点的超时时间短则不需要调整
      if (array_[parent]->expire_ <= timer->expire_) {
        break;
      }
      array_[hole] = array_[parent];
    }
    array_[hole] = timer;
  }


  // 删除目标定时器
  void del_tiemr(heap_timer* timer) {
    if (!timer) {
      return;
    }

    // 仅仅将目标定时器的回调函数设置为空，即所谓的延迟销毁。这将节省真正删除该定时器
    // 造成的开销，但这样做容易使堆数组膨胀
    timer->cb_func = NULL;
  }

  // 获得堆顶部的定时器
  heap_timer* top() const {
    if (empty()) {
      return NULL;
    }
    return array_[0];
  }

  // 删除堆顶部的定时器
  void pop_timer() {
    if (empty()) {
      return;
    }
    if (array_[0]) {
      delete array_[0];
      // 将原来的堆顶元素替换为对数组中最后一个元素
      array_[0] = array_[--cur_size_];
      
      // 对新的堆顶做下沉操作
      percolate_down(0);
    }
  }

  // 心搏函数（对外调用接口）
  void tick() {
    heap_timer* tmp = array_[0];
    time_t cur = time(NULL); // time 获取的是什么时间？绝对时间，时间戳形式
    while (!empty()) {
      if (!tmp) {
        break;
      }
      
      // 如果堆顶定时器没到期，则退出循环
      if (tmp->expire_ > cur) {
        break;
      }

      // 否则就执行堆顶定时器中的任务
      if (array_[0]->cb_func) {
        array_[0]->cb_func(array_[0]->user_data);
      }

      // 将堆顶元素删除，同时生成新的堆顶定时器
      pop_timer();
      tmp = array_[0];
    }
  }

  bool empty() const {return cur_size_ == 0;}

private:
  void percolate_down(int hole) {
    heap_timer* temp = array_[hole];
    int child = 0;
    for (;(hole * 2 + 1) <= (cur_size_ - 1); hole = child) {
      child = hole * 2 + 1;
      
      // 找到两个孩子中的较小值
      if (child < (cur_size_ - 1) && 
          (array_[child + 1]->expire_ < array_[child]->expire_)) {
        ++child;
      }

      // 将孩子节点中的较小值与当前节点进行比较
      if (array_[child]->expire_ < temp->expire_) {
        array_[hole] = array_[child];
      } else {
        break;
      }
    }

    // 将当前节点放入到正确的位置
    array_[hole] = temp;
  }

 // 将数组的容量扩大1倍
 void resize() {
  heap_timer** temp = new heap_timer*[2 * capacity_];
  for (int i = 0; i < 2 * capacity_; ++i) {
    temp[i] = NULL;
  }
  if (!temp) {
    throw std::exception();
  }
  capacity_ = 2 * capacity_;
  for (int i = 0; i < cur_size_; ++i) {
    temp[i] = array_[i];
  }
  delete[] array_;
  array_ = temp;
 }


 private:
 heap_timer **array_; // 堆数组
 int capacity_; // 堆数组的容量
 int cur_size_; // 堆数组当前包含的元素的个数


};








#endif