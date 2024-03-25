#include "SqlConnectionPool.h"


ConnectionPool::ConnectionPool() {
  this->free_connection_ = 0;
  this->cur_connection_ = 0;
}



// 通过返回静态局部变量的方式来获得单例对象
// 这是懒汉式单例模式最简单的实现方式
ConnectionPool* ConnectionPool::GetInstance() {
  static ConnectionPool connection_pool;
  return &connection_pool;
}

void ConnectionPool::Init(string url, int port, string user, string passwd, 
            string data_name, int max_connection) {
  this->url_ = url;
  this->port_ = port;
  this->user_ = user;
  this->passwd_ = passwd;
  this->database_name_ = data_name;
  
  // 初始化锁资源
  pthread_mutex_init(&this->lock_, NULL);

  // 创建 max_connection 个连接放入连接池
  for (int i = 0; i < max_connection; ++i) {
    // 初始化一个连接句柄
    MYSQL* connection = NULL;
    connection = mysql_init(connection);
    if (connection == NULL) {
      std::cout << "mysql_init error" << std::endl;
    }
  
    // 建立一个实际的连接
    connection = mysql_real_connect(connection, url.c_str(), user.c_str(), 
                                    passwd.c_str(), data_name.c_str(), port,
                                    NULL, 0);

    if (connection == NULL) {
      std::cout << "mysql_real_connect error" << std::endl;
      exit(-1);
    }

    // 放入连接池中
    connection_list_.push_back(connection);
    ++free_connection_;
  }

  // 初始化信号量的值为 free_connectoin, 表示资源的数量
  sem_init(&this->sem_, 0, this->free_connection_);
}

MYSQL* ConnectionPool::GetOneConnection() {
  MYSQL* connection = NULL;

  // 资源数量减1
  sem_wait(&sem_);
  // 对连接池的修改操作上锁，保证并发安全
  pthread_mutex_lock(&lock_);

  connection = connection_list_.front();
  connection_list_.pop_front();

  --free_connection_;
  ++cur_connection_;

  pthread_mutex_unlock(&lock_);
  return connection;
}

bool ConnectionPool::ReleaseOneConnection(MYSQL* connection) {
  if (NULL == connection)
    return false;

  pthread_mutex_lock(&lock_);

  connection_list_.push_back(connection);
  ++free_connection_;
  --cur_connection_;

  pthread_mutex_unlock(&lock_);
  sem_post(&sem_);
}


