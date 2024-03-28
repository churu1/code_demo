#include "log.h"
#include <string.h>
#include <time.h>

Log::Log() {
  count_ = 0;
  is_async_ = false; // 默认为同步写日志
}

Log::Log() {
  if (fp_ != NULL) {
    fclose(fp_);
  }
}

bool Log::Init(const char* file_name, int log_buf_size,
               int close_log, int split_line, int max_queue_size) {
  // 如果设置了 max_queue_size, 则为异步写日志
  if (max_queue_size >= 1) {
    // 创建写日志线程
    is_async_ = true;
    log_queue_ = new BlockQueue<string>(max_queue_size);
    pthread_t tid;
    pthread_create(&tid, NULL, threadFlushLog, NULL);
    pthread_detach(tid); // 当线程结束后自动释放线程资源，不需要主线程手动回收
  }

  // 给相关参数赋值
  log_buf_size_ = log_buf_size;
  split_lines_ = split_line;
  close_log_ = close_log;
  buf_ = new char[log_buf_size_];
  memset(buf_, '\0', log_buf_size_);

  // 创建一个时间结构体，用户获取时间
  time_t t = time(NULL);
  // 获取系统时间
  struct tm* sys_tm = localtime(&t);
  struct tm my_tm = *sys_tm;

  // 开始创建日志文件，日志文件名格式：YYYY_MM_DD_filename 的形式
  // 分离出日志文件名 找到 file_name 中最后一个 / 所在的位置右侧即是日志名
  const char* p = strrchr(file_name, '/');
  char log_full_name[256] = {0}; // 存储日志文件名称
  if (p == NULL) { // 在file_name中没有 '/',file_name 只是一个简单的文件名而包含路径
    // 那就在当前目录中创建日志文件即log_buff_name中不需要写入路径信息
    // tm_mdy 表示当前是一个月中的某一天
    // tm_wday 表示当前是这一周的某一天
    // tm_yday 表示当前是一年中的某一天
    snprintf(log_full_name, 255, "%d_%02d_%02d_%s", my_tm.tm_year + 1900, 
            my_tm.tm_mon + 1, my_tm.tm_mday, file_name);
  } else {
    // 如果 file_name 中包含路径，先将路径分离出来
    strncpy(dir_name_, file_name, p - file_name + 1); // 路径名
    strcpy(log_name_, p + 1); // 日志名
    // 完整的文件名 路径+创建时间+日志名
    snprintf(log_full_name, 255, "%s%d_%02d_%02d_%s", my_tm.tm_year + 1900, 
            my_tm.tm_mon + 1, my_tm.tm_mday, file_name, log_name_); 
  }

  // 记录当前是哪一天
  toady_ = my_tm.tm_mday + 1;

  // 以追加的形式打开日志文件，如果没有则创建文件
  fp_ = fopen(log_full_name, "a");
  // 创建失败
  if (fp_ == NULL) {
    return false;
  }

  return true;
}

// 写日志
void Log::WriteLog(int level, const char* format, ...) {
  struct timeval now = {0, 0};
  gettimeofday(&now, NULL);
  // 获取当前时间对应的秒数
  time_t t = now.tv_sec;
  // 将时间戳转换为本地时间
  struct tm* sys_tm = localtime(&t);
  struct tm my_tm = *sys_tm;
  char type[16] = {0};
  switch(level) {
    case 0 : {
      strcpy(type, "[debug]:");
      break;
    }
    case 1 : {
      strcpy(type, "[info]:");
      break;
    }
    case 2 : {
      strcpy(type, "[warn]:");
      break;
    }
    case 3 : {
      strcpy(type, "[erro]:");
      break;
    }
    default: {
      strcpy(type, "[info]:");
    }
  }

  // 每写入一条 log， 对 m_count++
  pthread_mutex_lock(&lock_);
  count_++;

  // 如果当前日期与记录日期不同或当个日志文件达到了设定的行数，需要重新床创建一个日志文件
  if (toady_ != my_tm.tm_mday + 1 || count_ % split_lines_ == 0) {
    char new_log_name[256] = {0};
    // 先把当前缓冲可能有的数据刷新到日志文件中
    fflush(fp_);
    // 关闭日志文件
    fclose(fp_);

    char data[16] = {0};
    snprintf(data, 16, "%d_%02d_%02d_", my_tm.tm_year + 1900, my_tm.tm_mon + 1,
             my_tm.tm_mday);    
    // 是新的一天的日志
    if (toady_ != my_tm.tm_mday + 1) {
      snprintf(new_log_name, 255, "%s%s%s", dir_name_, data, log_name_);
      // 记录新的一天
      toady_ = my_tm.tm_mday + 1;
      count_ = 0;
    } else {
      // 如果不是新的一天则是：日志记录条数达到上限，则将其分文件存储
      snprintf(new_log_name, 255, "%s%s%s.%lld", dir_name_, data, log_name_, 
              count_ / split_lines_);
    }
    fp_ = fopen(new_log_name, "a");
  }
  pthread_mutex_unlock(&lock_);

  // 声明可以接收可变数量参数的类型 va_lst;
  va_list va_lst;
  // 初始化 va_list 对象，第二参数为最后一个非可变参数类型的参数名
  va_start(va_lst, format);

  string single_log_str;

  pthread_mutex_lock(&lock_);

  int n = snprintf(buf_, 48, "%d-%02d--%02d %02d:%02d:%02d.%06ld %s",
                   my_tm.tm_yday + 1900, my_tm.tm_mon + 1, my_tm.tm_mday,
                   my_tm.tm_hour, my_tm.tm_min, my_tm.tm_sec, now.tv_sec, type);
  

  // 类似于 snprintf, 将 va_list 对象中的数据按照 format 格式写入 buf_ 中
  int m = vsnprintf(buf_ + n, log_buf_size_ - n - 1, format, va_lst);
  buf_[n + m] = '\n';
  buf_[n + m + 1] = '\0';
  single_log_str = buf_;

  pthread_mutex_unlock(&lock_);
  if (is_async_ && !log_queue_->isFull()) {
    log_queue_->Push(single_log_str);
  } else {
    pthread_mutex_lock(&lock_);
    fputs(single_log_str.c_str(), fp_);
    pthread_mutex_unlock(&lock_);
  }

  // 清除 va_list 对象
  va_end(va_lst);
}

void Log::Flush(void) {
  pthread_mutex_lock(&lock_);
  fflush(fp_);
  pthread_mutex_unlock(&lock_);
}