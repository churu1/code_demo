/*
循环数组实现阻塞队列：back_ = (back_ + 1) % max_size_
*/

#include <stdlib.h>
#include <pthread.h>
#include <sys/time.h>
#include <iostream>

template<typename T>
class BlockQueue {
 public:
  /// @brief 构造函数，初始化相关参数
  /// @param max_size 任务队列可以存储的最大任务的数量
  BlockQueue(int max_size = 1000) {
    if (max_size <= 0) {
      exit(-1);
    }

    // 初始化锁资源和条件变量
    pthread_mutex_init(&this->lock_, NULL);
    pthread_cond_init(&this->cond_, NULL);

    // 初始化相关参数
    this->max_size_ = max_size;
    this->array_ = new T[max_size];
    this->size_ = 0;
    this->back_ = -1;
    this->front_ = -1;
  }

  /// @brief 清空阻塞队列中的内容,不是真正上的清空，而是将相关参数初始化为
  /// 队列为空时的状态
  void Clear() {
    pthread_mutex_lock(&lock_);
    size_ = 0;
    front_ = -1;
    back_ = -1;
    pthread_mutex_unlock(&lock_);
  }

  ~BlockQueue() {
   pthread_mutex_lock(&lock_);
   if (array_ != nullptr) {
    delete[] array_;
   }
   ptherad_mutex_unlock(&lock_);
  }

  // 生产者添加一个任务
  bool Push(const T& item) {
    pthread_mutex_lock(&lock_);
    // 当前任务数量已经达到上限则通知消费者线程尽快拿取任务
    // 为什么不是多加一个条件变量，等待消费者消费一个任务后的条件变量
    if (size_ >= max_size_) {
      pthread_cond_broadcast(&cond_);
      pthread_mutex_unlock(&lock_);
      return false;
    }

    back_ = (back_ + 1) % max_size_;
    array_[back_] = item;

    ++size_;
    // 通知所有等待条件变量的线程，若当前没有线程等待，则唤醒无意义
    pthread_cond_broadcast(&cond_);
    pthread_mutex_unlock(&lock_);
    return true;
  }  

  // 消费者消费一个任务
  bool Pop(T& item) {
    pthread_mutex_lock(&lock_);
    // 当前没有任务，等待生产者生产一个任务
    while (size_ <= 0) {
      int ret = pthread_cond_wait(&cond_, &lock_);
      if (ret != 0) {
        pthread_mutex_unlock(&lock_);
        return false;
      }
    }

    // 取出一个任务
    front_ = (front_ + 1) % max_size_;
    item = array_[front_];
    --size_;
    pthread_mutex_unlock(&lock_);
    return true;
  }

  /// @brief 取出一个任务，如果没有任务可取则等待 ms_timeout 时间。
  /// @param item 存储任务的变量
  /// @param ms_timeout 超时时间
  /// @return 成功返回 ture 
  bool Pop(T& item, int ms_timeout) {
    struct timespec t = {0, 0};
    struct timeval now = {0, 0};
    gettimeofday(&now, NULL);
    pthread_mutex_lock(&lock_);
    if (size_ <= 0) {
      t.tv_sec = now.tv_sec + ms_timeout / 1000; //s
      t.tv_nsec = (ms_timeout % 1000) * 1000; // ns
      // 阻塞 ms_timeout 时间
      int ret = pthread_cond_timedwait(&cond_, &lock_, t);
      if (ret != 0) {
        pthread_mutex_unlock(&lock_);
        return false;
      }
    }

    // 阻塞 ms_timeout 时间后队列中仍然没有任务则直接返回失败
    if (size_ <= 0) {
      pthread_mutex_unlock(&lock_);
      return false;
    }

    // 正常取任务的逻辑
    front_ = (front_ + 1) % max_size_;
    item = array_[front_];
    --size_;
    pthread_mutex_unlock(&lock_);
    return true;
  }

  // 一些常用接口

  /// @brief 判断队列是否满了
  /// @return 满了返回true
  bool isFull() {
    pthread_mutex_lock(&lock_);
    if (size_ >= max_size_) {
      pthread_mutex_unlock(&lock_);
      return true;
    }    
    pthread_mutex_unlock(&lock_);
    return false;
  }

  bool isEmpty() {
    pthread_mutex_lock(&lock_);
    if (0 == size_) {
      pthread_mutex_unlock(&lock_);
      return true; 
    }

    pthread_mutex_unlock(&lock_);
    return false;
  }


 private:
  T* array_;
  int max_size_; // 阻塞队列中可以容纳的最大的任务的数量
  int size_; // 当前阻塞队列中任务的数量
  int front_; // 队头
  int back_; // 队尾


  pthread_mutex_t lock_; // 锁
  pthread_cond_t cond_; // 条件变量
};