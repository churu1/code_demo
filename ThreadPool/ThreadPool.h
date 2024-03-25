#include <pthread.h>
#include <unistd.h>
#include <malloc.h>
#include <string.h>
#include <signal.h>
#include <iostream>

#define TRUE  true
#define FALSE false
#define _DEF_COUNT 10
#define _DEF_TIMEOUT 10

void err_str(const char* str,int err)
{
    perror(str);
    exit(err);
}


typedef  struct {
  void* (*task)(void*);
  void* arg;
} task_t;

typedef struct STR_POOL_T {
  /// @brief 构造函数
  /// @param max_num 最大线程数量 
  /// @param min_num 最小线程数量
  /// @param que_max 任务队列的大小
  STR_POOL_T(int max_num, int min_num, int que_max);

  // 线程池相关参数
  int thread_max; // 最大线程数量
  int thread_min; // 最小线程数量
  int thread_busy; // 繁忙线程数量
  int thread_alive; // 存活的线程的数量
  int thread_wait; // 需要关闭的线程数量
  bool thread_shutdown; // 线程池关闭状态（初始化为TRUE, 表示开启）
  pthread_t* tids; // 用于描述线程池的线程数组
  pthread_t mamger_tid; // 管理者线程的线程id
  pthread_cond_t not_full; // 用于通知生产者可以继续生产的条件变量
  pthread_cond_t not_empty; // 用于通知消费者可以取任务的条件变量

  // 任务队列相关参数
  task_t* queue_task; // 任务队列
  int queue_max; // 最大任务数量
  int queue_cur_size; // 当前任务队列的任务数量
  int queue_front; // 任务队列对头
  int queue_rear; // 任务队列队尾
  pthread_mutex_t lock; // 用于锁住任务队列互斥锁
} pool_t;


// 线程池管理类
class ThreadPool {
 public:
  /// @brief 创建一个线程池
  /// @param  线程池的最大线程数
  /// @param  最小线程数
  /// @param  任务队列的最大任务数量
  /// @return 成功返回真
  bool CreatePool(int, int, int);
  
  /// @brief 销毁一个线程池
  void DestroyPool();
  
  /// @brief 生产者往任务队列中添加任务
  /// @param  任务的函数指针
  /// @param  任务的参数
  /// @return 成功返回0，失败返回-1
  int ProducerAdd(void*(*)(void*), void*);
  
  /// @brief 消费者从任务队列中取任务
  /// @param 线程工作函数的参数
  /// @return 一般没有返回值，因为线程的工作是一个死循环
  // 线程函数的类型为：void* (*cb_func)(void*)
  // 使用 static 的作用：1.让其不属于类对象，从而不会隐式加上 this 参数
  //                    2.让其具有与全局函数相同的行为，使之与线程工作函数的类型相同
  static void* Custom(void*);

  /// @brief 管理者线程，用于动态的管理线程池中线程的数量
  /// @param  线程工作函数的参数
  /// @return 一般没有返回值，因为线程的工作是一个死循环
  static void* Manager(void*);

  /// @brief 由静态的 Manager 函数调用，因此必须是静态的。用于判断
  ///        一个线程是否还存活
  /// @param  需要判断的线程的线程id
  /// @return 存活返回 TRUE 
  static bool IfThreadAlive(pthread_t);

 private:
  pool_t* pool_; // 线程池对象指针
};
