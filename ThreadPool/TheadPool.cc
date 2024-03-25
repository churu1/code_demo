#include "ThreadPool.h"

STR_POOL_T::STR_POOL_T(int max_num, int min_num, int que_max) {
  this->thread_max = max_num;
  this->thread_min = min_num;
  this->thread_busy = 0;
  this->thread_alive = 0;
  this->thread_wait = 0;
  this->thread_shutdown = TRUE;

  this->queue_max = que_max;
  this->queue_cur_size = 0;
  this->queue_rear = 0;
  this->queue_front = 0;

  // 初始化互斥锁和条件变量
  if (pthread_mutex_init(&this->lock, NULL) != 0 ||
      pthread_cond_init(&this->not_empty, NULL) != 0 ||
      pthread_cond_init(&this->not_full, NULL) != 0) {
    err_str("init cond or mutex error", -1);
  }

  // 申请线程数组空间 
  if ((this->tids = (pthread_t*)malloc(sizeof(pthread_t) * max_num)) == NULL) {
    err_str("malloc tids error:", -1);
  }
  // 初始化数组值为0
  memset(tids, 0, sizeof(pthread_t) * max_num);

  // 申请任务队列的空间
  if ((this->queue_task = (task_t*)malloc(sizeof(task_t) * que_max)) == NULL) {
    err_str("malloc taks queue error:", -1);
  }
}

// 创建线程池：
// 创建线程池对象，通过线程池对像构造函数为线程池初始化
// 创建 num_min 个线程，分配到线程池中
// 创建管理者线程
bool ThreadPool::CreatePool(int num_max, int num_min, int que_max) {
  // 创建线程池对象
  pool_ = new STR_POOL_T(num_max, num_min, que_max);

  int err = 0;
  for (int i = 0; i < num_min; ++i) {
    if ((err = pthread_create(&(pool_->tids[i]), NULL, Custom, 
        (void*)pool_)) > 0) {
      printf("create custom error:%s\n", strerror(err));
      return false;
    }
    // 存活的线程数加1
    ++(pool_->thread_alive);
  }

  if ((err = pthread_create(&(pool_->mamger_tid), NULL, Manager, 
      (void*)pool_)) > 0) {
      printf("create manger error:%s\n", strerror(err));
      return false;
  }
}

// 生产者往任务队列中添加任务
// 对任务队列的修改操作都上锁，添加任务完成后通知消费者
int ThreadPool::ProducerAdd( void*(*task)(void*arg), void* arg) {
  // 上锁
  pthread_mutex_lock(&pool_->lock);
  // 当任务队列已经满了，且线程池未关闭时，等待消费者的条件变量通知
  while (pool_->queue_cur_size == pool_->queue_max && pool_->thread_shutdown) {
    // 等待消费者的条件变量
    pthread_cond_wait(&pool_->not_full, &pool_->lock);
  }
  // 如果线程池是关闭的,则释放互斥锁资源并退出
  if (!pool_->thread_shutdown) {
    pthread_mutex_unlock(&pool_->lock);
    return -1;
  }

  // 任务队列不满且线程池未关闭,执行添加任务工作
  pool_->queue_task[pool_->queue_front].task = task;
  pool_->queue_task[pool_->queue_front].arg = arg;

  // 更新队头指针
  pool_->queue_front = (pool_->queue_front + 1) % pool_->queue_max;
  // 当前任务队列中的任务数加1
  ++(pool_->queue_cur_size);

  // 通知消费者线程有新的任务可取
  pthread_cond_signal(&pool_->not_empty);
  // 解锁
  pthread_mutex_unlock(&pool_->lock);
  return 0;
}

// 工作线程（消费者）函数，从任务队列中取出任务并执行
void* ThreadPool::Custom(void* arg) {
  // 获取线程池对象指针
  pool_t* p = (pool_t*)arg;
  task_t task;
  while (p->thread_shutdown) {
    // 上锁
    pthread_mutex_lock(&p->lock);

    // 如果任务队列为空且称线程池未关闭, 等待生产者通知
    // 再次判断线程池未关闭是为了防止在上锁完毕后线程池异常关闭
    while (p->queue_cur_size == 0 && p->thread_shutdown) {
      pthread_cond_wait(&p->not_empty, &p->lock);
    }

    // 如果线程池关闭了
    if (!p->thread_shutdown) {
      pthread_mutex_unlock(&p->lock);
      // 结束此线程
      pthread_exit(NULL);
    }

    // 等待关闭的线程数大于0，且当前存活的线程数大于最小线程数则结束此线程
    if (p->thread_wait > 0 && p->thread_alive > p->thread_min) {
      //等待关闭的线程数减1
      --(p->thread_wait);
      // 存活的线程数减1
      --(p->thread_alive);
      // 解锁
      pthread_mutex_unlock(&p->lock);
      // 结束此线程
      pthread_exit(NULL);
    }

    // 取出任务
    task.task = p->queue_task[p->queue_rear].task;
    task.arg = p->queue_task[p->queue_rear].arg;
    // 更新队列尾指针
    (p->queue_rear) = (p->queue_rear + 1) % p->queue_cur_size;
    // 任务队列中任务的数量减1 
    --(p->queue_cur_size);
    // 通知生产者线程可以添加新的任务
    pthread_cond_signal(&p->not_full); 
    // 繁忙线程数加1
    ++(p->thread_busy);
    // 解锁
    pthread_mutex_unlock(&p->lock);
    
    // 执行任务
    (*task.task)(task.arg);
    // 上锁
    pthread_mutex_lock(&p->lock);
    // 繁忙线程的数量减1
    --(p->thread_busy);
    // 解锁
    pthread_mutex_unlock(&p->lock);
  }
  return 0;
}

// 线程池管理线程函数
// 检测线程池中存活线程的数量，并根据存活数量，空闲数量，忙碌线程数量之间的关系
// 来动态调整线程池中的线程的数量
// 管理线程每执行一次就会 sleep 一定时间，节省 cpu 资源，而不是一直占用cpu
void* ThreadPool::Manager(void* arg) {
  // 获取线程池对象指针
  pool_t* p = (pool_t*)arg;
  
  // 存储线程池中相关属性的副本
  int alive = 0;
  int cur_size = 0;
  int busy = 0;
  int add = 0;
  
  while (p->thread_shutdown) {
    // 上锁
    pthread_mutex_lock(&p->lock);
    // 存活线程的数量
    alive = p->thread_alive;
    // 繁忙线程的线程数
    busy = p->thread_busy;
    // 当前任务队列汇中的任务数
    cur_size = p->queue_cur_size;
    // 解锁
    pthread_mutex_unlock(&p->lock);

    // 当前任务队列中的任务数大于存活线程的空闲数量，或繁忙线程的占比大于等于80%
    // 且存活的线程数小于最大线程数 增加新的工作线程。
    if ((cur_size > alive - busy) || (float)busy / alive * 100 >= (float)80 &&
        p->thread_max > alive) {

      // 一次性添加 thread_min 个新线程
      for (int j = 0; j < p->thread_min; ++j) {
        for (int i = 0; i < p->thread_max; ++i) {
          // 该线程不存在或已经结束
          if (p->tids[i] == 0 || !IfThreadAlive(p->tids[i])) {
            // 上锁
            pthread_mutex_lock(&p->lock);
            // 创建新的工作线程
            pthread_create(&p->tids[i], NULL, Custom, (void*)p);
            // 存活的线程数加1
            ++(p->thread_alive);
            // 解锁
            pthread_mutex_unlock(&p->lock);
            break;
          }
        }
      }
    }

    // 繁忙的线程数小于存活线程数的 1/3 ，且存活的线程数大于最小线程数
    // 用busy * 2 < alive - busy 来表示 < 1/3alive，秒啊
    if (busy * 2 < alive - busy && alive > p->thread_min) {
      // 上锁
      pthread_mutex_lock(&p->lock);
      // 设置需要等待关闭的线程数的默认值
      p->thread_wait = _DEF_COUNT;
      // 解锁
      pthread_mutex_unlock(&p->lock);
      // 通知工作线程有新的任务可以执行, 让其它们立即去执行多出来的任务
      for (int i = 0; i < _DEF_COUNT; ++i) {
        pthread_cond_signal(&p->not_empty);
      }
    }
    // 线程挂起一段时间，节省 cpu 资源
    sleep(_DEF_TIMEOUT);
  }
  return 0;
}

// 检查线程是否存活
// 通过将pthread_kill()函数的第二参数设为0，检查这个线程是否存活
// 如果线程不存在设置错误码，通常是 ESRCH
bool ThreadPool::IfThreadAlive(pthread_t tid) {
  if ((pthread_kill(tid, 0)) == -1) {
    if (errno == ESRCH) {
      return FALSE;
    }
  }
  return TRUE;
}