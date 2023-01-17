#ifndef _STUNNING_SCRIPT_MANAGER_H_
#define _STUNNING_SCRIPT_MANAGER_H_
#include <iostream>
#include <memory>
#include <mutex>
#include <string>

#include "global_define.hpp"
// lua头文件
#ifdef __cplusplus
extern "C" {
#include "lauxlib.h"
#include "lua.h"
#include "lualib.h"
}
#else
#include "lauxlib.h"
#include "lua.h"
#include "lualib.h"
#endif

#include <functional>
#include <queue>
#include <tuple>
#include <unordered_map>

namespace stunning {

enum ScriptMessageType {
  TYPE_CONSOLE = 1,                     // 控制台消息
  TYPE_CPP_MSG_CALL_DB = 2,             // 数据库回调
  TYPE_CPP_MSG_SOCKET_COMMAND = 3,      // socket消息
  TYPE_CPP_MSG_CONNECTION_ARRIVES = 4,  // 新连接
  TYPE_CPP_MSG_DISCONNECT = 5,          // 连接断开

};

class ScriptMessage {
 private:
  /* data */
 public:
  ScriptMessage(/* args */){};
  ~ScriptMessage(){};
  int type;           // ScriptMessageType 类型
  u_int16_t msg_len;  // socket消息
  u_int16_t code;     // socket消息
  std::string msg;    // socket消息, 控制台消息
  NetSocketId fd;     // socket的索引

  int cookie;              // 数据库的回调
  int query_flag;          // 执行有没有错
  int aff_rows;            // 影响行数
  std::string error_info;  // 错误信息
  bool is_query; // 是查询吗?
  std::vector<std::unordered_map<std::string, std::string>> result;  // 查询结果
};

class ScriptManager {
 private:
 public:
  ScriptManager(/* args */);
  ~ScriptManager();

  static ScriptManager* GetInstance();
  RetCode Init();

  void SetScriptMacro();
  std::mutex m_mtx;

  lua_State* _luaJitState;

  std::queue<std::shared_ptr<ScriptMessage>> wait_queue;

  RetCode AddMsg(std::shared_ptr<ScriptMessage> sm);
  RetCode BeforeProcessMessage();
  RetCode AfterProcessMessage();
  RetCode BeforeLuaLoop();
  RetCode LuaLoop();
  RetCode AfterLuaLoop();
};
}  // namespace stunning

#endif