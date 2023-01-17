/*
 * Filename: https://github.com/ccagml/stunning/src/database_manager.cpp
 * Path: https://github.com/ccagml/stunning/src
 * Created Date: Tuesday, January 17th 2023, 7:42:05 pm
 * Author: ccagml
 *
 * Copyright (c) 2023 ccagml . All rights reserved
 */

#include "database_manager.hpp"

#include "script_manager.hpp"
namespace stunning {

MysqlConn::MysqlConn(MYSQL* c, std::string name, bool error,
                     std::string error_i) {
  conn = c;
  db_name = name;
  is_error = error;
  error_info = error_i;
  // CCI("获取一个数据库链接")
}

MysqlConn::~MysqlConn() {
  // CCI("析构归还数据库链接");
}

DatabaseExec::DatabaseExec(int64_t c, std::string& db, std::string& sql) {
  cookie = c;
  db_name = db;
  sql_cmd = sql;
}
DatabaseExec::~DatabaseExec() {}

DatabaseManager::DatabaseManager() {
  boost::asio::thread_pool _db_worker_thread(10);
};
DatabaseManager::~DatabaseManager(){};

RetCode DatabaseManager::Init() { return NO_ERROR; }
DatabaseManager* DatabaseManager::GetInstance() {
  static DatabaseManager _ml_obj;
  return &_ml_obj;
}

RetCode DatabaseManager::AddExecDb(int64_t cookie, std::string& db_name,
                                   std::string& sql) {
  std::shared_ptr<DatabaseExec> sde =
      std::make_shared<DatabaseExec>(cookie, db_name, sql);
  boost::function<void(void)> f = [this, sde] { this->Call(sde); };
  boost::asio::post(_db_worker_thread, f);
  return NO_ERROR;
}

std::shared_ptr<MysqlConn> DatabaseManager::GetConn(std::string db_name) {
  std::unique_lock<std::mutex> lm(m_conn_mtx);
  MYSQL* a_conn;
  if (conn_pool.count(db_name) > 0) {
    std::queue<MYSQL*>& q = conn_pool[db_name];
    while (q.size() > 0) {
      a_conn = q.front();
      q.pop();
      if (!mysql_ping(a_conn)) {
        return std::make_shared<MysqlConn>(a_conn, db_name, false, "");
      } else {
        mysql_close(a_conn);
      }
    }
  }
  std::shared_ptr<ConfigDbInfo> db_cfg =
      ConfigManager::GetInstance()->getDbConfig(db_name);

  if (db_cfg != nullptr) {
    a_conn = mysql_init(NULL);

    if (!mysql_real_connect(a_conn, db_cfg->host.c_str(),
                            db_cfg->username.c_str(), db_cfg->passwd.c_str(),
                            db_name.c_str(), db_cfg->port, NULL, 0)) {
      std::cout << db_name.c_str() << ":" << mysql_error(a_conn);
      mysql_close(a_conn);
      return std::make_shared<MysqlConn>(nullptr, db_name, true,
                                         std::string(mysql_error(a_conn)));
    }
    char value = 1;
    mysql_options(a_conn, MYSQL_OPT_RECONNECT, (char*)&value);
    return std::make_shared<MysqlConn>(a_conn, db_name, false, "");
  }
  return std::make_shared<MysqlConn>(nullptr, db_name, true,
                                     db_name + "的数据库没有配置");
};

// 归还连接
void DatabaseManager::BackConn(std::shared_ptr<MysqlConn> mc) {
  std::unique_lock<std::mutex> lm(m_conn_mtx);
  if (mc->is_error) {
    return;
  }
  std::string db_name = mc->db_name;
  MYSQL* conn = mc->conn;
  // 5.5没有mysql_reset_connection 5.7才有
  int rest_flag = 0;
  // mysql_reset_connection(conn);
  unsigned long db_ver = mysql_get_server_version(conn);
  if (db_ver < 570000) {
    std::shared_ptr<ConfigDbInfo> db_cfg =
        ConfigManager::GetInstance()->getDbConfig(db_name);

    rest_flag = mysql_change_user(conn, db_cfg->username.c_str(),
                                  db_cfg->passwd.c_str(), db_name.c_str());
  } else {
    rest_flag = mysql_reset_connection(conn);
  }
  if (rest_flag == 0) {
    std::queue<MYSQL*>& q = conn_pool[db_name];
    q.push(conn);
    // CCI("将连接放入队列中 队列大小 %d", q.size())
  } else {
    std::cout << "关闭连接" << mysql_error(conn) << rest_flag;
    mysql_close(conn);
  }
}

void DatabaseManager::Call(std::shared_ptr<DatabaseExec> sde) {
  std::string db_name = sde->db_name;
  std::shared_ptr<MysqlConn> mc = GetConn(db_name);
  if (mc->is_error) {
    BackConn(mc);

    std::shared_ptr<ScriptMessage> ssm = std::make_shared<ScriptMessage>();
    ssm->type = TYPE_CPP_MSG_CALL_DB;
    ssm->cookie = sde->cookie;
    ssm->query_flag = -1;
    ssm->aff_rows = 0;
    ssm->error_info = mc->error_info;
    ssm->result = sde->result;
    ScriptManager::GetInstance()->AddMsg(ssm);

  } else {
    std::string sql_cmd = sde->sql_cmd;
    int query_flag = mysql_query(mc->conn, sql_cmd.c_str());
    MYSQL_RES* result;
    MYSQL_ROW row;
    MYSQL_FIELD* fields;
    sde->query_flag = query_flag;
    bool is_query = false;

    if (query_flag) {
      sde->error_info = std::string(mysql_error(mc->conn));
    } else {
      result = mysql_store_result(mc->conn);
      int num_fields;
      if (result == NULL) {
        if (mysql_field_count(mc->conn) == 0) {
          sde->aff_rows = mysql_affected_rows(mc->conn);

        } else {
          sde->error_info = std::string(mysql_error(mc->conn));
        }
      } else {
        num_fields = mysql_num_fields(result);
        fields = mysql_fetch_fields(result);
        std::vector<std::unordered_map<std::string, std::string>> row_result;
        while ((row = mysql_fetch_row(result))) {
          std::unordered_map<std::string, std::string> row_map;
          for (int i = 0; i < num_fields; i++) {
            if (row[i]) {
              row_map[fields[i].name] = row[i];
            }
          }
          row_result.push_back(std::move(row_map));
        }
        is_query = true;
        mysql_free_result(result);
        sde->result = row_result;
      }
    }
    BackConn(mc);
    // 把结果送回引擎

    std::shared_ptr<ScriptMessage> ssm = std::make_shared<ScriptMessage>();
    ssm->type = TYPE_CPP_MSG_CALL_DB;
    ssm->cookie = sde->cookie;
    ssm->query_flag = sde->query_flag;
    ssm->aff_rows = sde->aff_rows;
    ssm->error_info = mc->error_info;
    ssm->result = sde->result;
    ssm->is_query = is_query;
    ScriptManager::GetInstance()->AddMsg(ssm);
  }
}
}  // namespace stunning
