#ifndef _STUNNING_DATABASE_MANAGER_H_
#define _STUNNING_DATABASE_MANAGER_H_
#include <mysql.h>

#include <boost/asio.hpp>
#include <boost/log/trivial.hpp>
#include <boost/thread.hpp>
#include <boost/thread/thread.hpp>
#include <iostream>
#include <memory>
#include <mutex>
#include <string>

#include "config_manager.hpp"
#include "global_define.hpp"
#include "queue"
#include "unordered_map"
#include "vector"

namespace stunning {

class DatabaseExec {
 private:
  /* data */
 public:
  DatabaseExec(int64_t c, std::string& db, std::string& sql);
  ~DatabaseExec();
  int64_t cookie;
  std::string db_name;
  std::string sql_cmd;
  int query_flag;
  int64_t aff_rows;
  std::string error_info;
  std::vector<std::unordered_map<std::string, std::string>> result;
};

class MysqlConn {
 private:
  /* data */
 public:
  MysqlConn(MYSQL* c, std::string name, bool error, std::string error_info);
  ~MysqlConn();
  MYSQL* conn;
  std::string db_name;
  bool is_error;
  std::string error_info;
};

class DatabaseManager {
 private:
 public:
  DatabaseManager(/* args */);
  ~DatabaseManager();

  static DatabaseManager* GetInstance();
  RetCode Init();

  boost::asio::thread_pool _db_worker_thread;

  RetCode AddExecDb(int64_t cookie, std::string& db_name, std::string& sql);
  std::shared_ptr<MysqlConn> GetConn(std::string db_name);
  void Call(std::shared_ptr<DatabaseExec> sde);
  void BackConn(std::shared_ptr<MysqlConn> mc);
  std::mutex m_conn_mtx;

  std::unordered_map<std::string, std::queue<MYSQL*>> conn_pool;
};
}  // namespace stunning

#endif