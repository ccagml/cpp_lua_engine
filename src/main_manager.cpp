/*
 * Filename: https://github.com/ccagml/stunning/src/main_manager.cpp
 * Path: https://github.com/ccagml/stunning/src
 * Created Date: Tuesday, January 17th 2023, 7:42:05 pm
 * Author: ccagml
 *
 * Copyright (c) 2023 ccagml . All rights reserved
 */

#include "main_manager.hpp"

#include <boost/asio.hpp>

#include "config_manager.hpp"
#include "database_manager.hpp"
#include "net_manager.hpp"
#include "script_manager.hpp"

namespace stunning {
MainManager::MainManager() {
  boost::asio::thread_pool _console_worker_thread(1);
};
MainManager::~MainManager(){};

RetCode MainManager::Init(std::string &config_path, std::string &server_id) {
  RetCode ret = 0;
  ret = ConfigManager::GetInstance()->Init(config_path, server_id);
  if (ret) {
    return ret;
  }
  ret = NetManager::GetInstance()->Init();
  if (ret) {
    return ret;
  }
  ret = DatabaseManager::GetInstance()->Init();
  if (ret) {
    return ret;
  }
  ret = ScriptManager::GetInstance()->Init();
  if (ret) {
    return ret;
  }

  return NO_ERROR;
}

RetCode MainManager::Bind() {
  std::string listen_key = "listen";
  std::vector<std::string> vs =
      ConfigManager::GetInstance()->getConfigVS(listen_key);
  for (int i = 0; i < vs.size(); i++) {
    NetManager::GetInstance()->Bind(vs[i]);
  }
  return NO_ERROR;
}

// 每一帧
MsgRecvNum MainManager::Update() {
  std::cout << "MainManager update" << std::endl;
}

RetCode MainManager::Start() {
  NetManager::GetInstance()->Start();

  boost::function<void(void)> f = [this] {
    std::string cin_wait = "cin_wait";
    int sleep_time =
        std::atoi(ConfigManager::GetInstance()->getConfigS(cin_wait).c_str());
    std::cout << "cin wait" << sleep_time << std::endl;
    if (sleep_time < 1000) {
      sleep_time = 30000;
    }

    boost::this_thread::sleep(boost::posix_time::milliseconds(sleep_time));
    std::cin.clear();
    fflush(stdin);
    std::cout << "开始监听";
    while (true) {
      std::string sss;
      getline(std::cin, sss);

      std::shared_ptr<ScriptMessage> con_msg =
          std::make_shared<ScriptMessage>();

      con_msg->type = TYPE_CONSOLE;
      con_msg->msg = sss;
      ScriptManager::GetInstance()->AddMsg(con_msg);
      boost::this_thread::sleep(boost::posix_time::milliseconds(10));
    }
    return 0;
  };
  boost::asio::post(_console_worker_thread, f);

  while (true) {
    // post(g_pool, boost::bind(f, 100));
    // post(g_pool222, boost::bind(f1, 200));

    ScriptManager::GetInstance()->BeforeProcessMessage();
    ScriptManager::GetInstance()->AfterProcessMessage();
    ScriptManager::GetInstance()->BeforeLuaLoop();
    ScriptManager::GetInstance()->LuaLoop();
    ScriptManager::GetInstance()->AfterLuaLoop();
    boost::this_thread::sleep(boost::posix_time::milliseconds(1));
  }
  return NO_ERROR;
}

MainManager *MainManager::GetInstance() {
  static MainManager _ml_obj;
  return &_ml_obj;
}
}  // namespace stunning
// namespace stunning
