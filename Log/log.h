#include "block_queue.h"
#include <stdio.h>
#include <stdarg.h>
#include <pthread.h>
#include <iostream>

using std::string;

class Log {
  public:
  
  /// @brief 获取单例对象
  /// @return 返回一个单例对象
  static Log* GetInstance() {
    static Log instance;
    return &instance;
  }

  // 异步写日志线程的工作函数
  static void* threadFlushLog(void* arg) {
    // 调用异步写日志函数
    Log::GetInstance()->asyncWriteLog();
  }

  /// @brief 初始化函数
  /// @param file_name  日志文件名
  /// @param log_buf_size 单行日志缓冲区大小
  /// @param split_lines 单个日志文件的最大行数
  /// @param max_queue_size 阻塞队列的大小
  bool Init(const char* file_name, int close_log, int log_buf_size = 8192, 
            int split_lines = 5000000, int max_queue_size = 0);

  // 将单行日志写入日志文件
  void WriteLog(int level, const char* format, ...);

  // 刷新流缓冲区
  void Flush(void);


 private:
  // 将构造函数、析构函数私有化
  Log();
  ~Log();
  // 将拷贝构造，重载复制运算符函数删除
  Log(const Log& other) = delete;
  Log& operator=(const Log& other) = delete;

  // 异步写日志
  void* asyncWriteLog() {
    string single_log;
    // 从阻塞队列中取出一个日志，写入文件
    while (log_queue_->Pop(single_log)) {
      pthread_mutex_lock(&lock_);
      fputs(single_log.c_str(), fp_);
      pthread_mutex_unlock(&lock_);
    }
  }


  char dir_name_[128]; // 路径名
  char log_name_[128]; // log文件名
  int split_lines_; // 单个日志文件的最大行数
  int log_buf_size_; // 单行日志缓冲区的大小
  long long count_; // 日志记录的总数
  int toady_; // 记录当前是哪一天
  FILE* fp_; // 文件指针
  char* buf_; // 单行日志缓冲区
  int close_log_; // 是否关闭日志系统
  BlockQueue<string>* log_queue_; // 阻塞队列
  bool is_async_; // 同步异步标志，同步flase, 异步true
  pthread_mutex_t lock_; // 保护临界资源的锁
};

#define LOG_DEBUG(format, ...)\ 
        if(0 == close_log_) {\
        Log::GetInstance()->WriteLog(0, format, ##__VA_ARGS__);\
        Log::GetInstance()->Flush();}

#define LOG_INFO(format, ...)\ 
        if(0 == close_log_) {\
        Log::GetInstance()->WriteLog(1, format, ##__VA_ARGS__);\
        Log::GetInstance()->Flush();}

#define LOG_WARN(format, ...)\ 
        if(0 == close_log_) {\
        Log::GetInstance()->WriteLog(2, format, ##__VA_ARGS__);\
        Log::GetInstance()->Flush();}

#define LOG_ERROR(format, ...)\ 
        if(0 == close_log_) {\
        Log::GetInstance()->WriteLog(3, format, ##__VA_ARGS__);\
        Log::GetInstance()->Flush();}