// stunning.cpp: 定义应用程序的入口点。
//

#include "main.hpp"

#include <boost/asio.hpp>
#include <boost/program_options.hpp>
#include <iostream>
#include <string>

#include "main_manager.hpp"

// // lua头文件
// #ifdef __cplusplus
// extern "C" {
// #include "lauxlib.h"
// #include "lua.h"
// #include "lualib.h"
// }
// #else
// #include "lauxlib.h"
// #include "lua.h"
// #include "lualib.h"
// #endif

#include <mysql.h>

#include <boost/asio.hpp>
#include <boost/bind/bind.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/thread.hpp>
#include <boost/thread/thread.hpp>
#include <future>
#include <iostream>

#include "global_define.hpp"
#include "unordered_map"

boost::asio::thread_pool g_pool(20);
boost::asio::thread_pool g_pool222(2);
boost::asio::thread_pool g_pool_cin(1);

// int f(int i) {
//   std::cout << "(" + std::to_string(i) + ")";
//   return i * i;
// }

// int f1(int i) {
//   std::cout << "[" + std::to_string(i) + "]";
//   return i * i;
// }

int main(int argc, const char *argv[]) {
  boost::program_options::options_description desc("options");
  std::string config_file;
  std::string server_name;
  desc.add_options()("config",
                     boost::program_options::value<std::string>(&config_file)
                         ->value_name("config"))(
      "server_name", boost::program_options::value<std::string>(&server_name)
                         ->value_name("server_name"));

  boost::program_options::positional_options_description pd;
  boost::program_options::variables_map vm;
  boost::program_options::store(
      boost::program_options::command_line_parser(argc, argv)
          .options(desc)
          .positional(pd)
          .run(),
      vm);
  boost::program_options::notify(vm);
  stunning::RetCode ret =
      stunning::MainManager::GetInstance()->Init(config_file, server_name);
  if (ret) {
    return -1;
  }

  ret = stunning::MainManager::GetInstance()->Bind();
  if (ret) {
    std::cout << "Bind error";
    return -1;
  }
  ret = stunning::MainManager::GetInstance()->Start();
  if (ret) {
    std::cout << "Start error";
    return -1;
  }

  // lua_State *L;
  // L = luaL_newstate();
  // luaL_openlibs(L);
  // lua_pushnumber(L, 10);
  // lua_pushstring(L, "hello");
  // lua_pushboolean(L, 0);
  // lua_close(L);

  // MYSQL *a_conn = mysql_init(NULL);

  // std::string host = "192.168.1.1";
  // std::string username = "root";
  // std::string password = "123456";
  // std::string db_name = "hadb";
  // std::string sql_cmd = "select * from account_1";
  // int port = 3306;
  // bool error_flag =
  //     !mysql_real_connect(a_conn, host.c_str(), username.c_str(),
  //                         password.c_str(), db_name.c_str(), port, NULL, 0);
  // int query_flag = mysql_query(a_conn, sql_cmd.c_str());
  // MYSQL_RES *result;
  // MYSQL_ROW row;
  // result = mysql_store_result(a_conn);
  // int num_fields;
  // if (result == NULL) {
  //   if (mysql_field_count(a_conn) == 0) {
  //     int aff_rows = mysql_affected_rows(a_conn);
  //   } else {
  //     std::string error_info = std::string(mysql_error(a_conn));
  //   }
  // } else {
  //   num_fields = mysql_num_fields(result);
  //   MYSQL_FIELD *fields = mysql_fetch_fields(result);
  //   std::vector<std::unordered_map<std::string, std::string>> row_result;
  //   while ((row = mysql_fetch_row(result))) {
  //     std::unordered_map<std::string, std::string> row_map;
  //     for (int i = 0; i < num_fields; i++) {
  //       if (row[i]) {
  //         row_map[fields[i].name] = row[i];
  //       }
  //     }
  //     row_result.push_back(std::move(row_map));
  //   }
  //   mysql_free_result(result);
  // }

  // stunning::server::TcpServer s("0.0.0.0", "7788", "server1");

  // // Run the server until stopped.
  // s.run();

  // std::cout << std::unitbuf;
  // std::future<int> answer;

  // for (size_t i = 0; i != 50; ++i) {
  //   auto task = boost::bind(f, 10 * i);
  //   if (i == 42) {
  //     // answer = post(g_pool, std::packaged_task<int()>(task));
  //   } else {
  //     post(g_pool, boost::bind(f, 10 * i));
  //   }
  // }

  // for (size_t i = 0; i != 50; ++i) {
  //   post(g_pool222, boost::bind(f1, 10 * i));
  // }

  // post(g_pool_cin, boost::bind(my_cin));
  // // answer.wait();  // optionally make sure it is ready before blocking
  // // get() std::cout << "\n[Answer to #42: " + std::to_string(answer.get())
  // // + "]\n";

  // // wait for remaining tasks

  // while (true) {
  //   // post(g_pool, boost::bind(f, 100));
  //   // post(g_pool222, boost::bind(f1, 200));
  //   boost::this_thread::sleep(boost::posix_time::milliseconds(3000));
  // }
  return 0;
}
