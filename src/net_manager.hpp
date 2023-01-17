#ifndef _STUNNING_NET_MANAGER_H_
#define _STUNNING_NET_MANAGER_H_
#include <boost/asio.hpp>
#include <boost/log/trivial.hpp>
#include <boost/regex.hpp>
#include <boost/thread.hpp>
#include <boost/thread/thread.hpp>
#include <iostream>
#include <memory>
#include <mutex>
#include <string>

#include "global_define.hpp"
#include "queue"
#include "unordered_map"
#include "vector"

namespace stunning {

// 连接的基础类型
class BaseConnection : public std::enable_shared_from_this<BaseConnection> {
 public:
  virtual void start() = 0;
  virtual void stop() = 0;
  virtual void do_read() = 0;
  virtual void do_write() = 0;
  explicit BaseConnection(boost::asio::ip::tcp::socket socket);
  boost::asio::ip::tcp::socket socket_;
  NetSocketId _net_socket_id;
  /// Socket for the connection.
  virtual void do_write_cache(std::string &msg) = 0;
};

#define TCP_HEAD_MAGIC 0x54CC
#pragma pack(1)
struct TcpMsgHead {
  TcpMsgHead() : _magic(TCP_HEAD_MAGIC), _data_len(0), _action(0) {}
  u_int16_t _magic;
  u_int16_t _data_len;
  u_int16_t _action;
};

#pragma pack()

class TcpConnection : public BaseConnection {
 public:
  // boost::asio::ip::tcp::socket socket_;
  std::array<char, MaxConnectionBUffSize> buffer_;
  std::queue<std::string> _write_list;
  u_int16_t cur_read_len;
  std::mutex m_mtx;
  explicit TcpConnection(boost::asio::ip::tcp::socket socket);
  void start();
  void stop();
  void do_read();
  void do_write();
  void do_write_cache(std::string &msg);
  bool is_writing;
};

class WebSocket {
 public:
  std::string resource;
  std::string host;
  std::string origin;
  std::string protocol;
  std::string key;

  WebSocket();

  /**
   * @return [WS_INCOMPLETE_FRAME, WS_ERROR_FRAME, WS_OPENING_FRAME]
   */
  WEBSOCKET_FRAME_TYPE parseHandshake(std::string &str_msg);
  std::string answerHandshake();

  int32_t need_head_len(unsigned char *in_buffer, int in_length);
  int32_t need_body_len(unsigned char *in_buffer, int in_length);
  int makeFrame(WEBSOCKET_FRAME_TYPE frame_type, unsigned char *msg,
                int msg_len, unsigned char *buffer, int buffer_len);
  int makeFrameString(WEBSOCKET_FRAME_TYPE frame_type, std::string &str_msg,
                      std::string &out);
  WEBSOCKET_FRAME_TYPE getFrame(unsigned char *in_buffer, int in_length,
                                std::vector<unsigned char> &out, int *out_pos);
  WEBSOCKET_FRAME_TYPE isFrame(unsigned char *in_buffer, int in_length);

  std::string trim(std::string str);
  std::vector<std::string> explode(std::string theString,
                                   std::string theDelimiter,
                                   bool theIncludeEmptyStrings = false);
};

class WsConnection : public BaseConnection {
 public:
  // boost::asio::ip::tcp::socket socket_;
  std::array<char, MaxConnectionBUffSize> buffer_;
  std::queue<std::string> _write_list;
  u_int16_t cur_read_len;
  std::mutex m_mtx;
  explicit WsConnection(boost::asio::ip::tcp::socket socket);
  void start();
  void stop();
  void do_read();
  void do_write();
  void do_write_cache(std::string &msg);
  bool is_writing;
  std::shared_ptr<WebSocket> _ws_ptr;
  bool finish_hand_shake;
  std::shared_ptr<boost::asio::streambuf> hand_shake_first;
};
class HttpConnection : public BaseConnection {
 public:
  // boost::asio::ip::tcp::socket socket_;
  std::array<char, MaxConnectionBUffSize> buffer_;
  std::queue<std::string> _write_list;
  u_int16_t cur_read_len;
  std::mutex m_mtx;
  explicit HttpConnection(boost::asio::ip::tcp::socket socket);
  void start();
  void stop();
  void do_read();
  void do_write();
  void do_write_cache(std::string &msg);
  bool is_writing;
};

class SocketInfo {
 public:
  void Reset() {
    _socket_fd = -1;
    _addr_info = 0xffffffffUL;
    _ip = 0xffffffffUL;
    _port = 0xffff;
    _state = 0;
    ++_use_cnt;
    _reconnect_cnt = 0;
    _error_code = 0;
    _connection_ptr = nullptr;
  }

  int32_t _socket_fd;
  int32_t _last_socket_fd;  // 避免关闭了,不知道关了谁
  uint32_t _addr_info;      // 服务端这边的监听地址
  uint32_t _ip;             // 对方的ip
  uint16_t _port;           // 对方的端口
  uint32_t _state;          // 状态
  uint8_t _use_cnt;         // 利用了几次?重置++
  uint8_t _reconnect_cnt;   // 重连次数
  int32_t _error_code;      // 错误码
  std::shared_ptr<BaseConnection> _connection_ptr;
};

class NetManager {
 private:
  void accept_loop(std::shared_ptr<boost::asio::ip::tcp::acceptor> a,
                   LISTEN_STATE_TYPE listen_state_type);

 public:
  NetManager(/* args */);
  ~NetManager();

  std::shared_ptr<boost::asio::io_context> net_io_context_ptr;
  std::shared_ptr<boost::asio::ip::tcp::resolver> resolver_ptr;

  // 注册信号
  std::shared_ptr<boost::asio::signal_set> signals_ptr;

  boost::asio::thread_pool _io_worker_thread;

  /// 接受器用于侦听传入连接。
  // https://www.boost.org/doc/libs/1_81_0/doc/html/boost_asio/reference/ip__tcp/acceptor.html
  std::unordered_map<std::string,
                     std::shared_ptr<boost::asio::ip::tcp::acceptor>>
      _listen_acceptor;

  // 给Bind转换url
  RetCode UrlToNetAddress(const std::string &url, std::string *ip,
                          std::string *port);

  // 获取manager对象
  static NetManager *GetInstance();
  // 初始化
  RetCode Init();
  // 绑定监听
  RetCode Bind(std::string &url);

  // 开始io_context
  RetCode Start();

  RetCode io_run();

  RetCode CreateConn(std::string &url, int cookie);

  RetCode SendMessage(NetSocketId fd, std::string &msg);
  std::unordered_map<std::string, std::string> GetFdInfo(NetSocketId fd);

  // 获取_socket_id_pool中可用的索引编号;
  NetSocketId AllocNetAddr();

  std::vector<std::shared_ptr<SocketInfo>> _socket_id_pool;
  std::queue<NetSocketId> _free_socket_id_queue;
  int32_t _already_use_socket_id;

  std::shared_ptr<BaseConnection> ConnFactory(
      boost::asio::ip::tcp::socket socket, LISTEN_STATE_TYPE listen_state_type);
};
}  // namespace stunning

#endif