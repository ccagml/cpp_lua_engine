
#include "script_manager.hpp"

#include "config_manager.hpp"
#include "database_manager.hpp"
#include "main_manager.hpp"
#include "net_manager.hpp"
#include "script_protobuf.hpp"

namespace stunning {

ScriptManager::ScriptManager(){};
ScriptManager::~ScriptManager(){};

int luaopen_pb(lua_State *L) {
  luaL_newmetatable(L, IOSTRING_META);
  lua_pushvalue(L, -1);
  lua_setfield(L, -2, "__index");
  luaL_register(L, NULL, _c_iostring_m);

  luaL_register(L, "pb", _pb);
  return 1;
}

// 放入 std::vector<std::string>
void ccg_push_vec_s(lua_State *L, std::vector<std::string> &value) {
  lua_newtable(L);
  int counter = 0;
  for (const std::string &v : value) {
    lua_pushnumber(L, ++counter);
    lua_pushstring(L, v.c_str());
    lua_settable(L, -3);
  }
}

// 放入 std::vector<std::unordered_map<std::string, std::string>>
void ccg_push_vec_map_s_s(
    lua_State *L,
    std::vector<std::unordered_map<std::string, std::string>> &value) {
  lua_newtable(L);
  int counter = 0;
  for (const std::unordered_map<std::string, std::string> &list : value) {
    lua_pushnumber(L, ++counter);
    lua_newtable(L);
    for (auto const &item : list) {
      lua_pushstring(L, item.first.c_str());
      lua_pushstring(L, item.second.c_str());
      lua_settable(L, -3);
    }
    lua_settable(L, -3);
  }
}

// 放入 std::unordered_map<std::string, std::string> &value
void ccg_push_map_s_s(lua_State *L,
                      std::unordered_map<std::string, std::string> &value) {
  lua_newtable(L);
  for (auto const &item : value) {
    lua_pushstring(L, item.first.c_str());
    lua_pushstring(L, item.second.c_str());
    lua_settable(L, -3);
  }
}

// 发消息
int cpp_send_to_fd(lua_State *L) {
  TcpMsgHead p;

  NetSocketId fd = luaL_checkinteger(L, 1);  // First argument
  u_int16_t action_code = luaL_checkinteger(L, 2);
  std::string str_msg = luaL_checkstring(L, 3);
  int body_len = str_msg.size() + 6;
  std::vector<uint8_t> myVector(6);
  myVector[0] = 0;
  myVector[1] = 0;
  myVector[2] = body_len;
  myVector[3] = body_len >> 8;
  myVector[4] = action_code;
  myVector[5] = action_code >> 8;
  myVector.insert(myVector.end(), str_msg.begin(), str_msg.end());
  // myVector.append(str_msg.begin(), str_msg.end());
  std::string str(myVector.begin(), myVector.end());
  RetCode result = NetManager::GetInstance()->SendMessage(fd, str);
  lua_pushnumber(L, result);
  return 1;
}

int cpp_get_fd_info(lua_State *L) {
  NetSocketId fd = luaL_checkinteger(L, 1);  // First argument
  std::unordered_map<std::string, std::string> result =
      NetManager::GetInstance()->GetFdInfo(fd);
  ccg_push_map_s_s(L, result);
  return 1;
}

// 数据库语句
int cpp_call_db(lua_State *L) {
  int64_t cookie = luaL_checkinteger(L, 1);  // First argument
  std::string db_name = luaL_checkstring(L, 2);
  std::string db_sql = luaL_checkstring(L, 3);
  DatabaseManager::GetInstance()->AddExecDb(cookie, db_name, db_sql);
  return 0;
}

// 创建一个新链接
int cpp_new_connect(lua_State *L) {
  std::string ip = luaL_checkstring(L, 1);
  std::string port = luaL_checkstring(L, 2);
  int64_t cookie = luaL_checkinteger(L, 3);  // First argument

  std::string url = "tcp://" + ip + ":" + port;
  // std::cout << "c++创建一个新链接" << url << std::endl;
  NetManager::GetInstance()->CreateConn(url, cookie);
  return 0;
}

// 获取目录路径
int cpp_get_path(lua_State *L) {
  std::string temp = "root_path";
  std::string path = ConfigManager::GetInstance()->getConfigS(temp);
  lua_pushstring(L, path.c_str());
  return 1;
}
void ScriptManager::SetScriptMacro() {
  std::string macro = "global";
  std::string script_macro = ConfigManager::GetInstance()->getConfigS(macro);
  script_macro = script_macro + ",";
  int end_index = script_macro.size();
  int cur_line_index = 0;
  while (cur_line_index < end_index) {
    int next_line_index = script_macro.find(',', cur_line_index);

    std::string mac_key =
        script_macro.substr(cur_line_index, next_line_index - cur_line_index);
    std::string mac_value = ConfigManager::GetInstance()->getConfigS(mac_key);
    lua_pushstring(_luaJitState, mac_value.c_str());
    lua_setglobal(_luaJitState, mac_key.c_str());
    cur_line_index = next_line_index + 1;
  }
}

RetCode ScriptManager::Init() {
  MainManager::GetInstance()->Update();

  _luaJitState = luaL_newstate();
  luaL_openlibs(_luaJitState);

  lua_register(_luaJitState, "cpp_send_to_fd", cpp_send_to_fd);
  lua_register(_luaJitState, "cpp_call_db", cpp_call_db);
  lua_register(_luaJitState, "cpp_new_connect", cpp_new_connect);
  lua_register(_luaJitState, "cpp_get_path", cpp_get_path);
  lua_register(_luaJitState, "cpp_get_fd_info", cpp_get_fd_info);

  luaopen_pb(_luaJitState);
  SetScriptMacro();
  std::string init_path = "start_script";
  std::string init_file = ConfigManager::GetInstance()->getConfigS(init_path);
  std::string root_path = "root_path";
  std::string init_root = ConfigManager::GetInstance()->getConfigS(root_path);
  init_file = init_root + init_file;
  int ret = luaL_dofile(_luaJitState, init_file.c_str());
  if (ret != 0) {
    std::cout << luaL_checkstring(_luaJitState, -1) << std::endl;
    return -1;
  }
  lua_newtable(_luaJitState);
  lua_pcall(_luaJitState, 0, 0, 0);

  // luaL_register(this->se_luaState, NULL, regs);
  return NO_ERROR;
}
ScriptManager *ScriptManager::GetInstance() {
  static ScriptManager _ml_obj;
  return &_ml_obj;
}

RetCode ScriptManager::BeforeProcessMessage() {}
RetCode ScriptManager::AfterProcessMessage() {}
RetCode ScriptManager::BeforeLuaLoop() {
  int arg_cnt = -1;
  lua_getglobal(_luaJitState, "cpp_msg_xpcall_handler");
  lua_getglobal(_luaJitState, "cpp_msg_before_lua_loop");
  arg_cnt = 0;

  int error_index = -2 - arg_cnt;

  int status = lua_pcall(_luaJitState, arg_cnt, 0, error_index);
  if (status) {
    if (!lua_isnil(_luaJitState, -1)) {
      std::cout << "jit执行错了" << luaL_checkstring(_luaJitState, -1)
                << std::endl;
      lua_pop(_luaJitState, 1);
    }
  }
  // cpp_msg_xpcall_handler
  lua_pop(_luaJitState, 1);
}
RetCode ScriptManager::LuaLoop() {
  std::queue<std::shared_ptr<ScriptMessage>> temp_queue;
  {
    std::lock_guard<std::mutex> lock(m_mtx);
    while (wait_queue.size() > 0) {
      temp_queue.push(wait_queue.front());
      wait_queue.pop();
    }
  }

  while (temp_queue.size() > 0) {
    std::shared_ptr<ScriptMessage> cur = temp_queue.front();
    temp_queue.pop();

    int arg_cnt = -1;
    lua_getglobal(_luaJitState, "cpp_msg_xpcall_handler");
    switch (cur->type) {
      // 控制台直接执行指令
      case TYPE_CONSOLE:

        // 压入函数 cpp_msg_console
        lua_getglobal(_luaJitState, "cpp_msg_console");

        // 压入控制台 参数
        lua_pushlstring(_luaJitState, cur->msg.c_str(), cur->msg.size());
        arg_cnt = 1;
        break;
      case TYPE_CPP_MSG_CALL_DB:

        // 压入函数 cpp_msg_call_db
        lua_getglobal(_luaJitState, "cpp_msg_call_db");
        // 压入cookie
        lua_pushnumber(_luaJitState, cur->cookie);

        // 数据库执行错了
        if (cur->error_info.size() > 0) {
          lua_pushlstring(_luaJitState, cur->error_info.c_str(),
                          cur->error_info.size());
          arg_cnt = 2;
        } else {
          if (cur->is_query) {
            // 压入影响行数
            lua_pushnumber(_luaJitState, cur->result.size());
            // 压入查询结果
            ccg_push_vec_map_s_s(_luaJitState, cur->result);
            arg_cnt = 3;
          } else {
            lua_pushnumber(_luaJitState, cur->aff_rows);
            arg_cnt = 2;
          }
        }
        break;
      case TYPE_CPP_MSG_SOCKET_COMMAND:
        // 压入函数 cpp_msg_socket_command
        lua_getglobal(_luaJitState, "cpp_msg_socket_command");

        lua_pushnumber(_luaJitState, cur->fd);
        lua_pushnumber(_luaJitState, cur->code);
        lua_pushlstring(_luaJitState, cur->msg.c_str(), cur->msg.size());
        arg_cnt = 3;
        break;
      case TYPE_CPP_MSG_CONNECTION_ARRIVES:
        lua_getglobal(_luaJitState, "cpp_msg_connection_arrives");
        // 压入cookie
        lua_pushnumber(_luaJitState, cur->cookie);
        lua_pushnumber(_luaJitState, cur->fd);
        arg_cnt = 2;
        break;
      case TYPE_CPP_MSG_DISCONNECT:
        lua_getglobal(_luaJitState, "cpp_msg_disconnect");
        lua_pushnumber(_luaJitState, cur->fd);

        break;
      default:
        break;
    }
    int error_index = -2 - arg_cnt;

    int status = lua_pcall(_luaJitState, arg_cnt, 0, error_index);
    if (status) {
      if (!lua_isnil(_luaJitState, -1)) {
        std::cout << "jit执行错了" << luaL_checkstring(_luaJitState, -1)
                  << std::endl;
        lua_pop(_luaJitState, 1);
      }
    }
    // cpp_msg_xpcall_handler
    lua_pop(_luaJitState, 1);
  }
}
RetCode ScriptManager::AfterLuaLoop() {}
RetCode ScriptManager::AddMsg(std::shared_ptr<ScriptMessage> sm) {
  std::lock_guard<std::mutex> lock(m_mtx);
  wait_queue.push(sm);
}

}  // namespace stunning
