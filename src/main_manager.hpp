#ifndef _STUNNING_MAIN_MANAGER_H_
#define _STUNNING_MAIN_MANAGER_H_
#include <boost/asio.hpp>
#include <iostream>
#include <string>

#include "global_define.hpp"

namespace stunning {
class MainManager {
 private:
 public:
  MainManager(/* args */);
  ~MainManager();

  static MainManager *GetInstance();
  RetCode Init(std::string &config_path, std::string &server_id);
  RetCode Bind();
  RetCode Start();
  MsgRecvNum Update();

  boost::asio::thread_pool _console_worker_thread;
};
}  // namespace stunning

#endif