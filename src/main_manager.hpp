/*
 * Filename: https://github.com/ccagml/stunning/src/main_manager.hpp
 * Path: https://github.com/ccagml/stunning/src
 * Created Date: Tuesday, January 17th 2023, 7:42:05 pm
 * Author: ccagml
 *
 * Copyright (c) 2023 ccagml . All rights reserved
 */

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