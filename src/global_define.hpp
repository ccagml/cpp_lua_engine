/*
 * Filename: https://github.com/ccagml/stunning/src/global_define.hpp
 * Path: https://github.com/ccagml/stunning/src
 * Created Date: Tuesday, January 17th 2023, 7:42:05 pm
 * Author: ccagml
 *
 * Copyright (c) 2023 ccagml . All rights reserved
 */

#ifndef _STUNNING_GLOBAL_DEFINE_H_
#define _STUNNING_GLOBAL_DEFINE_H_

#include <stdint.h>

namespace stunning {

enum RET_CODE_ENUM {
  NO_ERROR = 0,
  NET_MANAGER_ERROR_START = -10000,
};

enum NET_MANAGER_ERROR_ENUM {
  NET_MANAGER_ERROR = NET_MANAGER_ERROR_START,
  NET_MANAGER_ERROR_BIND,
};

typedef int32_t RetCode;

typedef int32_t MsgRecvNum;

static const uint32_t MaxSendListSize = 1000;  // 等待发送的池子大小1000个

typedef uint64_t NetSocketId;  // 通过  AllocNetAddr  生成
static const NetSocketId INVAILD_NETADDR = UINT64_MAX;  // 无效地址
static const NetSocketId INVAILD_HANDLE = UINT64_MAX;   // 无效地址

static const int32_t MaxSocketNum = 5000;  // 最大socket5000个

static const int32_t MaxConnectionBUffSize = 8192;

typedef int32_t LISTEN_STATE_TYPE;

static const int32_t SOCKET_STATE_WS = (1 << 6);    // 第七位
static const int32_t SOCKET_STATE_HTTP = (1 << 5);  // 第六位
static const int32_t SOCKET_STATE_TCP = (1 << 4);   // 第五位
static const int32_t SOCKET_STATE_BLOCK =
    (1 << 3);  // 第四位 阻塞中,非阻塞115错误
static const int32_t SOCKET_STATE_NEED_CONNECT =
    (1 << 2);  // 第三位 // 等待连接
static const int32_t SOCKET_STATE_LISTEN = (1 << 1);  // 第二位 // 监听连接
static const int32_t SOCKET_STATE_ACCEPT = (1 << 0);  // 第一位 // 收到连接

enum WEBSOCKET_FRAME_TYPE {
  STUNNING_FRAME_INIT = 0x0000,
  ERROR_FRAME = 0xFF00,
  INCOMPLETE_FRAME = 0xFE00,

  OPENING_FRAME = 0x3300,
  CLOSING_FRAME = 0x3400,

  INCOMPLETE_TEXT_FRAME = 0x01,
  INCOMPLETE_BINARY_FRAME = 0x02,

  TEXT_FRAME = 0x81,
  BINARY_FRAME = 0x82,

  PING_FRAME = 0x19,
  PONG_FRAME = 0x1A,
  CLOSE_FRAME = 0x08
};
enum ScriptMessage_TYPE {
  TYPE_NUMBER = 1,
  TYPE_STRING = 2,
};

}  // namespace stunning
#endif