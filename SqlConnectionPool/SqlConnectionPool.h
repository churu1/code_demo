#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#include <mysql/mysql.h>
#include <error.h>
#include <string.h>
#include <iostream>
#include <list>

using  std::list;
using  std::string;

class ConnectionPool {
 public:

  /// @brief 获取线程池的单例对象
  /// @return 线程池的单例对象
  static ConnectionPool* GetInstance(); 

  /// @brief 从连接池中获得一个数据库连接对象并修改连接池相关参数
  /// @return 返回一个数据库连接对象
  MYSQL* GetOneConnection();

  /// @brief 将某个数据库连接重新放回连接池中并修改连接池
  /// @param connection 需要放回连接池中的数据库连接 
  /// @return 成功返回 true
  bool ReleaseOneConnection(MYSQL* connection);

  /// @brief 获得连接池中空闲连接的数量
  /// @return 返回连接池中空闲连接的数量
  int GetFreeConnection();

  /// @brief 销毁线程池中所有的连接
  void DestroyPool();

  /// @brief 初始化一个数据库连接池
  /// @param url 数据库的主机名
  /// @param port 端口
  /// @param user 用户名
  /// @param passwd 用户密码
  /// @param data_name 数据库名称
  /// @param max_connection 数据库连接池中最大连接个数
  void Init(string url, int port, string user, string passwd, 
            string data_name, int max_connection);
  private:
   // 将构造函数、拷贝构造、重载=运算符函数都设为私有函数以实现单例模式
   ConnectionPool();
   ~ConnectionPool();
   ConnectionPool(const ConnectionPool& other);
   ConnectionPool& operator=(const ConnectionPool& other);


 private:
  // 数据库相关参数
  string url_; // 数据库的主机名
  int port_; // 数据库端口
  string user_; // 用户名
  string passwd_; // 用户密码
  string database_name_; // 数据库的名称

  // 数据库连接池相关参数
  // int max_connection_; // 连接池中最大的连接个数
  int cur_connection_; // 连接池中以使用的连接个数
  int free_connection_; // 连接池中空闲的连接个数
  pthread_mutex_t lock_; // 用于保护链表的锁
  sem_t sem_; // 表示连接池中可用连接的数量
  list<MYSQL*> connection_list_; // 用于描述连接池的链表
};

/// @brief 对数据库连接池的一层封装，用于自动管理连接池对象
/// 创建对象时即获取资源并初始化，离开作用域自动释放资源
class ConnectionRAII {
 public:
  /// @brief 从数据库连接池 connection_pool 中取得一个数据库连接 
  /// @param connection 二级指针，修改空数据库对象指针的指向，让其指向
  /// 连接池中的某个数据库连接对象。因此为二级指针
  /// @param connection_pool 连接池对象
  ConnectionRAII(MYSQL** connection, ConnectionPool* connection_pool);
  ~ConnectionRAII();

 private:
  MYSQL* connRAII_; // 数据库连接对象
  ConnectionPool* poolRAII_;// 连接池对象
};
