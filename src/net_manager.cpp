/*
 * Filename: https://github.com/ccagml/stunning/src/net_manager.cpp
 * Path: https://github.com/ccagml/stunning/src
 * Created Date: Tuesday, January 17th 2023, 7:42:05 pm
 * Author: ccagml
 * 
 * Copyright (c) 2023 ccagml . All rights reserved
 */



#include "net_manager.hpp"

#include "script_manager.hpp"
#include "sha1.h"
namespace stunning {
static const char *ws = " \t\n\r\f\v";

// trim from end of string (right)
inline std::string &rtrim(std::string &s, const char *t = stunning::ws) {
  s.erase(s.find_last_not_of(t) + 1);
  return s;
}

// trim from beginning of string (left)
inline std::string &ltrim(std::string &s, const char *t = stunning::ws) {
  s.erase(0, s.find_first_not_of(t));
  return s;
}

// trim from both ends of string (right then left)
inline std::string &trim(std::string &s, const char *t = stunning::ws) {
  return ltrim(rtrim(s, t), t);
}

WebSocket::WebSocket() {}

WEBSOCKET_FRAME_TYPE WebSocket::parseHandshake(std::string &str_msg) {
  std::string headers(str_msg);
  std::string::size_type header_end = headers.find("\r\n\r\n");

  if (header_end == std::string::npos) {
    return INCOMPLETE_FRAME;
  }

  headers.resize(header_end);
  std::vector<std::string> headers_rows = explode(headers, std::string("\r\n"));
  int headers_rows_n = headers_rows.size();
  for (int i = 0; i < headers_rows_n; i++) {
    std::string &header = headers_rows[i];
    if (header.find("GET") == 0) {
      std::vector<std::string> get_tokens = explode(header, std::string(" "));
      if (get_tokens.size() >= 2) {
        this->resource = get_tokens[1];
      }
    } else {
      std::string::size_type pos = header.find(":");
      if (pos != std::string::npos) {
        std::string header_key(header, 0, pos);
        std::string header_value(header, pos + 1);
        header_value = stunning::trim(header_value);
        if (header_key == "Host")
          this->host = header_value;
        else if (header_key == "Origin")
          this->origin = header_value;
        else if (header_key == "Sec-WebSocket-Key")
          this->key = header_value;
        else if (header_key == "Sec-WebSocket-Protocol")
          this->protocol = header_value;
      }
    }
  }

  return OPENING_FRAME;
}

std::vector<std::string> WebSocket::explode(std::string theString,
                                            std::string theDelimiter,
                                            bool theIncludeEmptyStrings) {
  // printf("EXPLODE\n");
  // UASSERT( theDelimiter.size(), >, 0 );

  std::vector<std::string> theStringVector;
  int length = 0;
  std::string::size_type start = 0, end = 0;

  while (end != std::string::npos) {
    end = theString.find(theDelimiter, start);

    // If at end, use length=maxLength.  Else use length=end-start.
    length = (end == std::string::npos) ? std::string::npos : end - start;

    if (theIncludeEmptyStrings ||
        ((length > 0) /* At end, end == length == string::npos */
         && (start < theString.size())))
      theStringVector.push_back(theString.substr(start, length));

    // If at end, use start=maxSize.  Else use start=end+delimiter.
    start = ((end > (std::string::npos - theDelimiter.size()))
                 ? std::string::npos
                 : end + theDelimiter.size());
  }
  return theStringVector;
}

std::string WebSocket::answerHandshake() {
  unsigned char digest[20];  // 160 bit sha1 digest

  std::string answer;
  answer += "HTTP/1.1 101 Switching Protocols\r\n";
  answer += "Upgrade: WebSocket\r\n";
  answer += "Connection: Upgrade\r\n";
  if (this->key.length() > 0) {
    std::string accept_key;
    accept_key += this->key;
    accept_key += "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";  // RFC6544_MAGIC_KEY

    // printf("INTERMEDIATE_KEY:(%s)\n", accept_key.data());

    SHA1 sha;
    sha.Input(accept_key.data(), accept_key.size());
    sha.Result((unsigned *)digest);

    // little endian to big endian
    for (int i = 0; i < 20; i += 4) {
      unsigned char c;

      c = digest[i];
      digest[i] = digest[i + 3];
      digest[i + 3] = c;

      c = digest[i + 1];
      digest[i + 1] = digest[i + 2];
      digest[i + 2] = c;
    }

    // printf("DIGEST:"); for(int i=0; i<20; i++) printf("%02x ",digest[i]);
    // printf("\n");

    accept_key = sha.base64_encode((const unsigned char *)digest,
                                   20);  // 160bit = 20 bytes/chars

    answer += "Sec-WebSocket-Accept: " + (accept_key) + "\r\n";
  }
  if (this->protocol.length() > 0) {
    answer += "Sec-WebSocket-Protocol: " + (this->protocol) + "\r\n";
  }
  answer += "\r\n";
  return answer;
}

int WebSocket::makeFrameString(WEBSOCKET_FRAME_TYPE frame_type,
                               std::string &str_msg, std::string &out) {
  int pos = 0;
  int size = str_msg.size();
  out.push_back((unsigned char)frame_type);  // text frame

  if (size <= 125) {
    // buffer[pos++] = size;
    out.push_back((unsigned char)size);
  } else if (size <= 65535) {
    // buffer[pos++] = 126; //16 bit length follows
    out.push_back((unsigned char)126);
    out.push_back((unsigned char)((size >> 8) & 0xFF));
    out.push_back((unsigned char)(size & 0xFF));
    // buffer[pos++] = (size >> 8) & 0xFF; // leftmost first
    // buffer[pos++] = size & 0xFF;
  } else {  // >2^16-1 (65535)
    // buffer[pos++] = 127; //64 bit length follows
    out.push_back((unsigned char)127);
    // write 8 bytes length (significant first)

    // since msg_length is int it can be no longer than 4 bytes = 2^32-1
    // padd zeroes for the first 4 bytes
    for (int i = 3; i >= 0; i--) {
      // buffer[pos++] = 0;
      out.push_back((unsigned char)0);
    }
    // write the actual 32bit msg_length in the next 4 bytes
    for (int i = 3; i >= 0; i--) {
      out.push_back((unsigned char)((size >> 8 * i) & 0xFF));
      // buffer[pos++] = ((size >> 8 * i) & 0xFF);
    }
  }
  // memcpy((void *)(buffer + pos), msg, size);
  out.append(str_msg.begin(), str_msg.end());
  return (size + pos);
}

WEBSOCKET_FRAME_TYPE WebSocket::getFrame(unsigned char *in_buffer,
                                         int in_length,
                                         std::vector<unsigned char> &out,
                                         int *out_pos) {
  // printf("getTextFrame()\n");
  if (in_length < 2) return INCOMPLETE_FRAME;

  //   0                   1                   2                   3
  //   0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
  //  +-+-+-+-+-------+-+-------------+-------------------------------+
  //  |F|R|R|R| opcode|M| Payload len |    Extended payload length    |
  //  |I|S|S|S|  (4)  |A|     (7)     |             (16/64)           |
  //  |N|V|V|V|       |S|             |   (if payload len==126/127)   |
  //  | |1|2|3|       |K|             |                               |
  //  +-+-+-+-+-------+-+-------------+ - - - - - - - - - - - - - - - +
  //  |     Extended payload length continued, if payload len == 127  |
  //  + - - - - - - - - - - - - - - - +-------------------------------+
  //  |                               |Masking-key, if MASK set to 1  |
  //  +-------------------------------+-------------------------------+
  //  | Masking-key (continued)       |          Payload Data         |
  //  +-------------------------------- - - - - - - - - - - - - - - - +
  //  :                     Payload Data continued ...                :
  //  + - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - +
  //  |                     Payload Data continued ...                |
  //  +---------------------------------------------------------------+

  unsigned char msg_opcode = in_buffer[0] & 0x0F;
  unsigned char msg_fin = (in_buffer[0] >> 7) & 0x01;
  unsigned char msg_masked = (in_buffer[1] >> 7) & 0x01;
  //     ???OPCODE??????????????????

  // ????????????0x0?????????????????????
  // ????????????0x1?????????????????????
  // ????????????0x2????????????????????????
  // ????????????0x3 - 7????????????????????????????????????????????????
  // ????????????0x8??????????????????
  // ????????????0x9??????ping
  // ????????????0xA??????pong
  // ????????????0xB - F?????????????????????????????????????????????
  // *** message decoding

  int payload_length = 0;
  int pos = 2;
  int length_field = in_buffer[1] & (~0x80);
  unsigned int mask = 0;
  // MASK:
  //     1??????????????????PayloadData????????????????????????????????????????????????????????????????????????????????????????????????1????????????????????????

  // ??????Payload length == = x?????????

  // ???????????? x??????0 - 125?????????payload??????????????????
  // ????????????
  // x??????126????????????2??????????????????16??????????????????????????????payload??????????????????
  // ????????????
  // x??????127????????????8??????????????????64??????????????????????????????payload??????????????????

  // ?????????????????????payload length??????????????????????????????payload
  // length????????????????????????????????????big endian??????????????????????????? printf("IN:");
  // for(int i=0; i<20; i++) printf("%02x ",buffer[i]); printf("\n");

  if (length_field <= 125) {
    payload_length = length_field;
  } else if (length_field == 126) {  // msglen is 16bit!
    // payload_length = in_buffer[2] + (in_buffer[3]<<8);
    payload_length = ((in_buffer[2] << 8) | (in_buffer[3]));
    pos += 2;
  } else if (length_field == 127) {  // msglen is 64bit!
    payload_length =
        ((((uint64_t)in_buffer[2]) << 56) | (((uint64_t)in_buffer[3]) << 48) |
         (((uint64_t)in_buffer[4]) << 40) | (((uint64_t)in_buffer[5]) << 32) |
         (in_buffer[6] << 24) | (in_buffer[7] << 16) | (in_buffer[8] << 8) |
         (in_buffer[9]));
    pos += 8;
  }

  // printf("PAYLOAD_LEN: %08x\n", payload_length);
  if (in_length < payload_length + pos) {
    return INCOMPLETE_FRAME;
  }

  if (msg_masked) {
    mask = *((unsigned int *)(in_buffer + pos));
    // printf("MASK: %08x\n", mask);
    pos += 4;

    // unmask data:
    unsigned char *c = in_buffer + pos;
    for (int i = 0; i < payload_length; i++) {
      c[i] = c[i] ^ ((unsigned char *)(&mask))[i % 4];
    }
  }

  // std::string msg;
  // msg.append(reinterpret_cast<char *>(in_buffer + pos), payload_length);
  // memcpy((void *)out_buffer, (void *)(in_buffer + pos), payload_length);
  // out.resize(payload_length);
  // for (int start = pos; start < pos + payload_length; start++)
  // {
  //     out[start] = &(in_buffer + start);
  // }
  // out(in_buffer + pos, in_buffer + pos + payload_length);
  std::copy(in_buffer + pos, in_buffer + pos + payload_length,
            std::back_inserter(out));
  *out_pos = pos;
  // printf("TEXT: %s\n", out_buffer);

  if (msg_opcode == 0x0)
    return (msg_fin) ? TEXT_FRAME
                     : INCOMPLETE_TEXT_FRAME;  // continuation frame ?
  if (msg_opcode == 0x1) return (msg_fin) ? TEXT_FRAME : INCOMPLETE_TEXT_FRAME;
  if (msg_opcode == 0x2)
    return (msg_fin) ? BINARY_FRAME : INCOMPLETE_BINARY_FRAME;
  if (msg_opcode == 0x9) return PING_FRAME;
  if (msg_opcode == 0xA) return PONG_FRAME;
  if (msg_opcode == 0X8) {
    return CLOSE_FRAME;
  }

  return ERROR_FRAME;
}

// ??????????????????????????? ??????0??????????????????
int32_t WebSocket::need_head_len(unsigned char *in_buffer, int in_length) {
  // ????????????2??????
  if (in_length < 2) {
    return 2 - in_length;
  }

  int pos = 2;
  int length_field = in_buffer[1] & (~0x80);
  if (length_field <= 125) {
  } else if (length_field == 126) {
    pos += 2;
  } else if (length_field == 127) {
    pos += 8;
  }
  return pos - in_length;
}

int32_t WebSocket::need_body_len(unsigned char *in_buffer, int in_length) {
  if (in_length < 2) {
    return 0;
  }

  unsigned char msg_masked = (in_buffer[1] >> 7) & 0x01;
  int payload_length = 0;
  int pos = 2;
  int length_field = in_buffer[1] & (~0x80);
  if (length_field <= 125) {
    payload_length = length_field;
  } else if (length_field == 126) {
    payload_length = ((in_buffer[2] << 8) | (in_buffer[3]));
    pos += 2;
  } else if (length_field == 127) {  // msglen is 64bit!
    payload_length =
        ((((uint64_t)in_buffer[2]) << 56) | (((uint64_t)in_buffer[3]) << 48) |
         (((uint64_t)in_buffer[4]) << 40) | (((uint64_t)in_buffer[5]) << 32) |
         (in_buffer[6] << 24) | (in_buffer[7] << 16) | (in_buffer[8] << 8) |
         (in_buffer[9]));
    pos += 8;
  }
  if (msg_masked) {
    pos += 4;
  }

  return (payload_length + pos) - in_length;
}

WEBSOCKET_FRAME_TYPE WebSocket::isFrame(unsigned char *in_buffer,
                                        int in_length) {
  // printf("getTextFrame()\n");
  if (in_length < 2) return INCOMPLETE_FRAME;

  unsigned char msg_opcode = in_buffer[0] & 0x0F;
  unsigned char msg_fin = (in_buffer[0] >> 7) & 0x01;
  // unsigned char msg_masked = (in_buffer[1] >> 7) & 0x01;

  int payload_length = 0;
  int pos = 2;
  int length_field = in_buffer[1] & (~0x80);
  // unsigned int mask = 0;

  if (length_field <= 125) {
    payload_length = length_field;
  } else if (length_field == 126) {  // msglen is 16bit!
    // payload_length = in_buffer[2] + (in_buffer[3]<<8);
    payload_length = ((in_buffer[2] << 8) | (in_buffer[3]));
    pos += 2;
  } else if (length_field == 127) {  // msglen is 64bit!
    payload_length =
        ((((uint64_t)in_buffer[2]) << 56) | (((uint64_t)in_buffer[3]) << 48) |
         (((uint64_t)in_buffer[4]) << 40) | (((uint64_t)in_buffer[5]) << 32) |
         (in_buffer[6] << 24) | (in_buffer[7] << 16) | (in_buffer[8] << 8) |
         (in_buffer[9]));
    pos += 8;
  }

  // printf("PAYLOAD_LEN: %08x\n", payload_length);
  if (in_length < payload_length + pos) {
    return INCOMPLETE_FRAME;
  }

  // printf("TEXT: %s\n", out_buffer);

  if (msg_opcode == 0x0)
    return (msg_fin) ? TEXT_FRAME
                     : INCOMPLETE_TEXT_FRAME;  // continuation frame ?
  if (msg_opcode == 0x1) return (msg_fin) ? TEXT_FRAME : INCOMPLETE_TEXT_FRAME;
  if (msg_opcode == 0x2)
    return (msg_fin) ? BINARY_FRAME : INCOMPLETE_BINARY_FRAME;
  if (msg_opcode == 0x9) return PING_FRAME;
  if (msg_opcode == 0xA) return PONG_FRAME;

  return ERROR_FRAME;
}

// 10009	boost::asio::error::bad_descriptor
// ?????????????????????????????????????????????async_receive()

// 995	boost::asio::error::operation_aborted
// ??????????????????cance()l????????????
// ??????async_receive()?????????????????????????????????????????????

// 10054	boost::asio::error::connection_reset  ???????????????????????????????????????
// ??????async_receive()?????????????????????????????????TCP???????????????RESET????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????

// 2	boost::asio::error::eof  ??????????????????close()?????????
// ??????async_receive()?????????????????????????????????????????????????????????10054??????????????????????????????????????????????????????????????????????????????????????????????????????jack???????????????????????????????????????????????????

BaseConnection::BaseConnection(boost::asio::ip::tcp::socket socket)
    : socket_(std::move(socket)) {}

TcpConnection::TcpConnection(boost::asio::ip::tcp::socket socket)
    : BaseConnection(std::move(socket)) {}

void TcpConnection::start() { do_read(); }
void TcpConnection::stop() {
  socket_.shutdown(boost::asio::ip::tcp::socket::shutdown_receive);
  socket_.close();
}
void TcpConnection::do_read() {
  auto self(shared_from_this());

  // ??????????????????
  u_int16_t need_read_len = 0;
  u_int16_t head_len = sizeof(TcpMsgHead);
  // ???????????????
  if (cur_read_len < head_len) {
    need_read_len = head_len - cur_read_len;
  } else {
    // ?????????
    TcpMsgHead *msg_head = (TcpMsgHead *)buffer_.data();
    size_t body_len = msg_head->_data_len;
    need_read_len = (body_len)-cur_read_len;
  }
  // std::cout << "???????????? cur_read_len:" << cur_read_len
  //           << ":head_len:" << head_len << ":need_read_len:" << need_read_len
  //           << std::endl;

  socket_.async_read_some(
      boost::asio::buffer(buffer_.begin() + cur_read_len, need_read_len),
      [this, self](boost::system::error_code ec,
                   std::size_t bytes_transferred) {
        std::string temp =
            std::string(buffer_.data(), cur_read_len + bytes_transferred);

        // std::cout << "?????? ??????:" << bytes_transferred << std::endl;
        if (!ec) {
          cur_read_len += bytes_transferred;

          // ????????????????????????
          u_int16_t head_len = sizeof(TcpMsgHead);
          if (cur_read_len >= head_len) {
            TcpMsgHead *msg_head = (TcpMsgHead *)buffer_.data();
            size_t body_len = msg_head->_data_len;
            size_t msg_need_len = body_len;
            if (cur_read_len == msg_need_len) {
              std::string temp =
                  std::string(buffer_.data() + head_len, body_len - head_len);
              std::cout << "????????????1??????"
                        << "??????" << msg_head->_magic
                        << ":??????:" << msg_head->_data_len << ":??????"
                        << msg_head->_action << "??????:" << cur_read_len
                        << ":??????" << temp << std::endl;

              std::shared_ptr<ScriptMessage> ss =
                  std::make_shared<ScriptMessage>();
              ss->type = TYPE_CPP_MSG_SOCKET_COMMAND;
              ss->fd = _net_socket_id;
              ss->code = msg_head->_action;
              ss->msg = temp;
              ss->msg_len = body_len - head_len;
              ScriptManager::GetInstance()->AddMsg(ss);
              cur_read_len = 0;
            }
          }
          do_read();
        } else if (ec != boost::asio::error::operation_aborted) {
          std::cout << "ec != boost::asio::error::operation_aborted"
                    << std::endl;
        } else {
          std::cout << "else boost::asio::error::operation_aborted";
        }
      });
  // socket_.async_read_some(
  //     boost::asio::buffer(buffer_),
  //     [this, self](boost::system::error_code ec,
  //                  std::size_t bytes_transferred) {
  // std::cout << " ??????tcp??????============" << ec
  //           << ":bytes_transferred:" << bytes_transferred <<
  //           "message()"
  //           << ec.message() << std::endl;
  // if (!ec) {
  //   std::string temp = std::string(buffer_.data(), bytes_transferred);
  //   std::cout << "??????" << temp << "??????:" << bytes_transferred
  //             << std::endl;
  //   do_read();
  // } else if (ec != boost::asio::error::operation_aborted) {
  //   std::cout << "ec != boost::asio::error::operation_aborted"
  //             << std::endl;
  // } else {
  //   std::cout << "else boost::asio::error::operation_aborted";
  // }
  //     });
}

void TcpConnection::do_write_cache(std::string &msg) {
  {
    std::lock_guard<std::mutex> lock(m_mtx);
    _write_list.push(msg);
  }
  do_write();
}

void TcpConnection::do_write() {
  auto self(shared_from_this());
  std::string msg;
  {
    std::lock_guard<std::mutex> lock(m_mtx);
    if (!is_writing && _write_list.size() > 0) {
      msg = std::move(_write_list.front());
      _write_list.pop();
      is_writing = true;
    } else {
      // std::cout << "?????? do_write ????????????" << msg << std::endl;
    }
  }
  if (msg != "") {
    boost::asio::async_write(
        socket_, boost::asio::buffer(msg, msg.size()),
        [this, self](boost::system::error_code ec, std::size_t write_size) {
          if (!ec) {
            std::cout << "??????" << write_size << "??????" << std::endl;
            {
              std::lock_guard<std::mutex> lock(m_mtx);
              is_writing = false;
            }
            do_write();
          }
          if (ec != boost::asio::error::operation_aborted) {
          }
        });
  }
}

WsConnection::WsConnection(boost::asio::ip::tcp::socket socket)
    : BaseConnection(std::move(socket)) {
  _ws_ptr = std::make_shared<WebSocket>();
  finish_hand_shake = false;
  hand_shake_first = std::make_shared<boost::asio::streambuf>(1024);
}

void WsConnection::start() { do_read(); }
void WsConnection::stop() {
  socket_.shutdown(boost::asio::ip::tcp::socket::shutdown_receive);
  socket_.close();
}

void WsConnection::do_read() {
  auto self(shared_from_this());
  if (!finish_hand_shake) {
    boost::asio::async_read_until(
        socket_, *hand_shake_first, boost::regex("\r\n\r\n"),
        [this, self](boost::system::error_code ec,
                     std::size_t bytes_transferred) {
          std::string hand_msg(
              boost::asio::buffers_begin(hand_shake_first->data()),
              boost::asio::buffers_begin(hand_shake_first->data()) +
                  bytes_transferred);
          std::cout << "ws ??????" << ec
                    << "bytes_transferred:" << bytes_transferred
                    << "hand_msg:" << hand_msg << std::endl;
          WEBSOCKET_FRAME_TYPE ws_check = _ws_ptr->parseHandshake(hand_msg);
          if (ws_check == OPENING_FRAME) {
            std::string ans_msg = _ws_ptr->answerHandshake();
            boost::asio::async_write(
                socket_, boost::asio::buffer(ans_msg, ans_msg.size()),
                [this, self](boost::system::error_code ec,
                             std::size_t write_size) {
                  if (!ec) {
                    std::cout << "??????" << write_size << "??????,????????????"
                              << std::endl;
                    {
                      std::lock_guard<std::mutex> lock(m_mtx);
                      finish_hand_shake = true;
                    }
                    do_write();
                    do_read();
                  }
                  if (ec != boost::asio::error::operation_aborted) {
                  }
                });
          } else {
            stop();
            std::cout << "ws ?????? ??????" << std::endl;
          }
        });

  } else {
    if (MaxConnectionBUffSize - cur_read_len <= 0) {
      stop();
      return;
    }

    socket_.async_read_some(
        boost::asio::buffer(buffer_.begin() + cur_read_len,
                            MaxConnectionBUffSize - cur_read_len),
        [this, self](boost::system::error_code ec,
                     std::size_t bytes_transferred) {
          std::cout << "ws????????????:" << bytes_transferred << "?????????:" << ec
                    << std::endl;
          if (!ec) {
            cur_read_len += bytes_transferred;

            std::vector<unsigned char> client_msg;
            // ???????????????????????????
            for (int iii = 0; iii < 10; iii++) {
              client_msg.clear();
              int out_pos = 0;
              WEBSOCKET_FRAME_TYPE ws_type =
                  _ws_ptr->getFrame((unsigned char *)(&buffer_[0]),
                                    cur_read_len, client_msg, &out_pos);
              std::cout << "ws ????????????:" << ws_type << std::endl;
              if (ws_type == ERROR_FRAME || ws_type == CLOSE_FRAME) {
                std::cout << "???????????????";
                stop();
                return;
              } else if (ws_type == TEXT_FRAME || ws_type == BINARY_FRAME) {
                // ????????????????????????
                std::cout << "????????????";
                u_int16_t head_len = sizeof(TcpMsgHead);
                if (client_msg.size() >= head_len) {
                  TcpMsgHead *msg_head = (TcpMsgHead *)client_msg.data();
                  size_t body_len = msg_head->_data_len;
                  size_t msg_need_len = body_len;
                  if (client_msg.size() == msg_need_len) {
                    std::string temp = std::string(
                        client_msg.begin() + head_len, client_msg.end());

                    std::shared_ptr<ScriptMessage> ss =
                        std::make_shared<ScriptMessage>();
                    ss->type = TYPE_CPP_MSG_SOCKET_COMMAND;
                    ss->fd = _net_socket_id;
                    ss->code = msg_head->_action;
                    ss->msg = temp;
                    ss->msg_len = body_len - head_len;
                    ScriptManager::GetInstance()->AddMsg(ss);

                    int32_t msg_end = out_pos + client_msg.size();
                    std::copy(buffer_.begin() + msg_end,
                              buffer_.begin() + cur_read_len, buffer_.begin());
                    cur_read_len -= msg_end;

                    std::cout << "ws ????????????1??????"
                              << ":????????????:" << msg_head->_data_len
                              << "   action:  " << msg_head->_action
                              << "   out_pos:   " << out_pos
                              << "   out:    " << client_msg.size()
                              << "   new cur_read_len:" << cur_read_len
                              << std::endl;
                  }

                } else if (ws_type == INCOMPLETE_FRAME ||
                           ws_type == INCOMPLETE_TEXT_FRAME ||
                           ws_type == INCOMPLETE_BINARY_FRAME) {
                  std::cout << "????????????:" << ws_type;

                  break;
                }
              }
            }
            do_read();
          } else if (ec != boost::asio::error::operation_aborted) {
            std::cout << "ec != boost::asio::error::operation_aborted"
                      << std::endl;
          } else {
            std::cout << "else boost::asio::error::operation_aborted";
          }
        });
  };
}

void WsConnection::do_write_cache(std::string &msg) {
  std::string process_data = "";
  _ws_ptr->makeFrameString(BINARY_FRAME, msg, process_data);
  {
    std::lock_guard<std::mutex> lock(m_mtx);
    _write_list.push(process_data);
  }
  do_write();
}

void WsConnection::do_write() {
  if (!finish_hand_shake) {
    std::cout << "ws ???????????????" << std::endl;
    return;
  }
  auto self(shared_from_this());
  std::string msg;
  {
    std::lock_guard<std::mutex> lock(m_mtx);
    if (!is_writing && _write_list.size() > 0) {
      msg = std::move(_write_list.front());
      _write_list.pop();
      is_writing = true;
    } else {
      // std::cout << "?????? do_write ????????????" << msg << std::endl;
    }
  }
  if (msg != "") {
    boost::asio::async_write(
        socket_, boost::asio::buffer(msg, msg.size()),
        [this, self](boost::system::error_code ec, std::size_t write_size) {
          if (!ec) {
            std::cout << "??????" << write_size << "??????" << std::endl;
            {
              std::lock_guard<std::mutex> lock(m_mtx);
              is_writing = false;
            }
            do_write();
          }
          if (ec != boost::asio::error::operation_aborted) {
          }
        });
  }
}

HttpConnection::HttpConnection(boost::asio::ip::tcp::socket socket)
    : BaseConnection(std::move(socket)) {}

void HttpConnection::start() { do_read(); }
void HttpConnection::stop() {
  socket_.shutdown(boost::asio::ip::tcp::socket::shutdown_receive);
  socket_.close();
}
void HttpConnection::do_read() {
  auto self(shared_from_this());
  socket_.async_read_some(
      boost::asio::buffer(buffer_),
      [this, self](boost::system::error_code ec,
                   std::size_t bytes_transferred) {
        std::cout << " ??????http??????============" << ec
                  << ":bytes_transferred:" << bytes_transferred << "message()"
                  << ec.message() << std::endl;
        if (!ec) {
          std::string temp = std::string(buffer_.data(), bytes_transferred);
          std::cout << "??????" << temp << "??????:" << bytes_transferred
                    << std::endl;
          do_read();
        } else if (ec != boost::asio::error::operation_aborted) {
          std::cout << "ec != boost::asio::error::operation_aborted"
                    << std::endl;
        } else {
          std::cout << "else boost::asio::error::operation_aborted";
        }
      });
}

void HttpConnection::do_write_cache(std::string &msg) {
  {
    std::lock_guard<std::mutex> lock(m_mtx);
    _write_list.push(msg);
    if (!is_writing && _write_list.size() == 1) {
      is_writing = true;
      do_write();
    }
  }
}
void HttpConnection::do_write() {
  auto self(shared_from_this());
  boost::asio::async_write(
      socket_, boost::asio::buffer(buffer_),
      [this, self](boost::system::error_code ec, std::size_t) {
        if (!ec) {
          // Initiate graceful connection closure.
          boost::system::error_code ignored_ec;
          socket_.shutdown(boost::asio::ip::tcp::socket::shutdown_both,
                           ignored_ec);
        }

        if (ec != boost::asio::error::operation_aborted) {
        }
      });
}

NetManager::NetManager() {
  net_io_context_ptr = std::make_shared<boost::asio::io_context>();

  resolver_ptr = std::make_shared<boost::asio::ip::tcp::resolver>(
      net_io_context_ptr->get_executor());

  signals_ptr = std::make_shared<boost::asio::signal_set>(
      net_io_context_ptr->get_executor());
  boost::asio::thread_pool _io_worker_thread(1);
};
NetManager::~NetManager(){};

void NetManager::accept_loop(std::shared_ptr<boost::asio::ip::tcp::acceptor> a,
                             LISTEN_STATE_TYPE listen_state_type) {
  a->async_accept(a->get_executor(), [a, listen_state_type](
                                         boost::system::error_code ec,
                                         boost::asio::ip::tcp::socket &&s) {
    if (!ec) {
      std::cout << "accepted " << s.remote_endpoint()
                << " on :" << a->local_endpoint().port() << std::endl;

      NetSocketId new_id = NetManager::GetInstance()->AllocNetAddr();
      if (new_id == INVAILD_NETADDR) {
        BOOST_LOG_TRIVIAL(error)
            << "?????????????????? accepted " << s.remote_endpoint()
            << " on :" << a->local_endpoint().port();
        s.close();
      }

      std::shared_ptr<SocketInfo> ss =
          NetManager::GetInstance()
              ->_socket_id_pool[static_cast<uint32_t>(new_id)];
      ss->_connection_ptr = NetManager::GetInstance()->ConnFactory(
          std::move(s), listen_state_type);

      ss->_connection_ptr->_net_socket_id = new_id;
      ss->_connection_ptr->start();

      std::shared_ptr<ScriptMessage> ss_msg = std::make_shared<ScriptMessage>();
      ss_msg->type = TYPE_CPP_MSG_CONNECTION_ARRIVES;
      ss_msg->fd = new_id;
      ScriptManager::GetInstance()->AddMsg(ss_msg);

      NetManager::GetInstance()->accept_loop(a, listen_state_type);
    } else {
      std::cout << "exit accept_loop " << ec.message() << std::endl;
    }
  });
}

RetCode NetManager::SendMessage(NetSocketId fd, std::string &msg) {
  std::shared_ptr<SocketInfo> ss =
      NetManager::GetInstance()->_socket_id_pool[static_cast<uint32_t>(fd)];
  if (ss->_connection_ptr != nullptr) {
    ss->_connection_ptr->do_write_cache(msg);
    return NO_ERROR;
  } else {
    std::cout << "?????????" << fd << std::endl;
    return 1;
  }
}

std::unordered_map<std::string, std::string> NetManager::GetFdInfo(
    NetSocketId fd) {
  std::shared_ptr<SocketInfo> ss =
      NetManager::GetInstance()->_socket_id_pool[static_cast<uint32_t>(fd)];

  std::unordered_map<std::string, std::string> result;

  boost::asio::ip::tcp::endpoint local_end =
      ss->_connection_ptr->socket_.local_endpoint();
  boost::asio::ip::tcp::endpoint remote_end =
      ss->_connection_ptr->socket_.remote_endpoint();

  result["local_address"] = local_end.address().to_string();
  int lp = local_end.port();
  result["local_port"] = std::to_string(lp);
  result["remote_address"] = remote_end.address().to_string();
  int rp = remote_end.port();
  result["remote_port"] = std::to_string(rp);
  return result;
}

RetCode NetManager::Init() {
  // ???????????????????????????????????????
  // ?????????????????????????????????????????????????????????
  // ????????????
  signals_ptr->add(SIGINT);
  signals_ptr->add(SIGTERM);
  signals_ptr->add(SIGQUIT);
  signals_ptr->add(
      SIGPIPE);  // ??????????????????,??????socket???????????? Broken pipe ???????????????

  signals_ptr->async_wait(
      [this](boost::system::error_code /*ec*/, int signal /*signo*/) {
        // ?????????????????????????????????????????????
        // ????????? ?????????????????????????????? io_context::run()
        // ??????????????????
        switch (signal) {
          case SIGINT:
            std::cout << "SIGNINT" << std::endl;
            break;
          case SIGTERM:
            std::cout << "SIGTERM" << std::endl;
            break;
          case SIGQUIT:
            std::cout << "SIGQUIT" << std::endl;
            break;
          case SIGPIPE:
            std::cout << "SIGPIPE" << std::endl;
            break;
          default:
            std::cout << "default signal :" << signal << std::endl;
            break;
        }
      });

  // ?????????socketInfo
  for (int32_t si = 0; si < MaxSocketNum; si++) {
    _socket_id_pool.push_back(std::make_shared<SocketInfo>());
  }
  return NO_ERROR;
}

// ???????????????  _use_cnt / _socket_id_pool ?????????   // 64??? ???32???????????????
// _use_cnt ??????32???????????????_socket_id_pool?????????
NetSocketId NetManager::AllocNetAddr() {
  NetSocketId ret = INVAILD_NETADDR;
  if (_free_socket_id_queue.size() > 0) {
    ret = _free_socket_id_queue.front();
    _free_socket_id_queue.pop();
  } else if (_already_use_socket_id < MaxSocketNum) {
    ret = (_already_use_socket_id++);
    _socket_id_pool[ret]->Reset();
    ret |= (static_cast<uint64_t>(_socket_id_pool[ret]->_use_cnt) << 32);
  }
  return ret;
}

RetCode NetManager::UrlToNetAddress(const std::string &url, std::string *ip,
                                    std::string *port) {
  if (NULL == ip || NULL == port) {
    return NET_MANAGER_ERROR;
  }

  size_t pos = url.find_last_of(':');
  if (std::string::npos == pos || url.size() == pos) {
    return NET_MANAGER_ERROR;
  }

  ip->assign(url.substr(0, pos));
  *port = url.substr(pos + 1);
  return NO_ERROR;
}

RetCode NetManager::Bind(std::string &url) {
  std::string ip;
  std::string cut_ip;
  std::string port;

  if (UrlToNetAddress(url, &ip, &port) != 0) {
    BOOST_LOG_TRIVIAL(error) << "Bind url:" << url;
    return NET_MANAGER_ERROR_BIND;
  }

  // ????????????????????????????????????
  LISTEN_STATE_TYPE listen_type = 0;
  if (0 == ip.compare(0, 6, "tcp://")) {
    cut_ip = ip.substr(6);
    listen_type |= SOCKET_STATE_TCP;
  } else if (0 == ip.compare(0, 7, "http://")) {
    cut_ip = ip.substr(7);
    listen_type |= SOCKET_STATE_HTTP;
  } else if (0 == ip.compare(0, 5, "ws://")) {
    cut_ip = ip.substr(5);
    listen_type |= SOCKET_STATE_WS;
  } else {
    cut_ip = ip;
    listen_type |= SOCKET_STATE_TCP;
  }

  boost::asio::ip::tcp::endpoint endpoint(
      boost::asio::ip::address_v4::from_string(cut_ip),
      std::atoi(port.c_str()));

  //??? Boost C++
  //?????????boost::asio::ip::tcp::acceptor????????????????????????????????????????????????????????????????????????IP
  //?????????????????????????????????????????????????????????acceptor?????????????????????????????????????????????????????????????????????????????????

  std::shared_ptr<boost::asio::ip::tcp::acceptor> new_acceptor_ptr =
      std::make_shared<boost::asio::ip::tcp::acceptor>(
          net_io_context_ptr->get_executor());

  boost::system::error_code ec;
  new_acceptor_ptr->open(endpoint.protocol(), ec);
  new_acceptor_ptr->set_option(
      boost::asio::ip::tcp::acceptor::reuse_address(true));
  new_acceptor_ptr->bind(endpoint);
  new_acceptor_ptr->listen();
  _listen_acceptor[url] = std::move(new_acceptor_ptr);

  // ???acceptor?????????socket??????

  accept_loop(_listen_acceptor[url], listen_type);
  std::cout << "????????????" << url << std::endl;
}

NetManager *NetManager::GetInstance() {
  static NetManager _ml_obj;
  return &_ml_obj;
}

RetCode NetManager::Start() {
  boost::function<void(void)> f = [this] { net_io_context_ptr->run(); };
  boost::asio::post(_io_worker_thread, f);

  return NO_ERROR;
}

std::shared_ptr<BaseConnection> NetManager::ConnFactory(
    boost::asio::ip::tcp::socket socket, LISTEN_STATE_TYPE listen_state_type) {
  if (listen_state_type & SOCKET_STATE_TCP) {
    std::shared_ptr<TcpConnection> s_con_ptr =
        std::make_shared<TcpConnection>(std::move(socket));
    return s_con_ptr;
  } else if (listen_state_type & SOCKET_STATE_WS) {
    std::shared_ptr<WsConnection> s_con_ptr =
        std::make_shared<WsConnection>(std::move(socket));
    return s_con_ptr;
  } else if (listen_state_type & SOCKET_STATE_HTTP) {
    std::shared_ptr<HttpConnection> s_con_ptr =
        std::make_shared<HttpConnection>(std::move(socket));
    return s_con_ptr;
  }
  return nullptr;
}

// ?????????????
RetCode NetManager::CreateConn(std::string &url, int cookie) {
  std::string ip;
  std::string cut_ip;
  std::string port;

  if (UrlToNetAddress(url, &ip, &port) != 0) {
    BOOST_LOG_TRIVIAL(error) << "Bind url:" << url;
    return NET_MANAGER_ERROR_BIND;
  }

  // ????????????????????????????????????
  LISTEN_STATE_TYPE listen_type = 0;
  if (0 == ip.compare(0, 6, "tcp://")) {
    cut_ip = ip.substr(6);
    listen_type |= SOCKET_STATE_TCP;
  } else if (0 == ip.compare(0, 7, "http://")) {
    cut_ip = ip.substr(7);
    listen_type |= SOCKET_STATE_HTTP;
  } else if (0 == ip.compare(0, 5, "ws://")) {
    cut_ip = ip.substr(5);
    listen_type |= SOCKET_STATE_WS;
  } else {
    cut_ip = ip;
    listen_type |= SOCKET_STATE_TCP;
  }

  boost::asio::ip::tcp::endpoint endpoint(
      boost::asio::ip::address_v4::from_string(cut_ip),
      std::atoi(port.c_str()));

  boost::asio::ip::tcp::socket new_socket(net_io_context_ptr->get_executor());

  NetSocketId new_id = NetManager::GetInstance()->AllocNetAddr();
  if (new_id == INVAILD_NETADDR) {
    std::cout << "????????????" << std::endl;
  }

  std::shared_ptr<SocketInfo> socket_info_ptr =
      NetManager::GetInstance()->_socket_id_pool[static_cast<uint32_t>(new_id)];

  socket_info_ptr->_connection_ptr = NetManager::GetInstance()->ConnFactory(
      std::move(new_socket), listen_type);

  socket_info_ptr->_connection_ptr->socket_.async_connect(
      endpoint,
      [this, url, new_id, cookie](const boost::system::error_code &error) {
        if (!error) {
          // Connect succeeded.
          std::cout << "????????????" << url << ":new_id:" << new_id << std::endl;
          std::shared_ptr<ScriptMessage> sps =
              std::make_shared<ScriptMessage>();
          sps->type = TYPE_CPP_MSG_CONNECTION_ARRIVES;
          sps->cookie = cookie;
          sps->fd = new_id;
          ScriptManager::GetInstance()->AddMsg(sps);

          std::shared_ptr<SocketInfo> ss =
              NetManager::GetInstance()
                  ->_socket_id_pool[static_cast<uint32_t>(new_id)];
          ss->_connection_ptr->_net_socket_id = new_id;
          ss->_connection_ptr->start();
        } else {
          std::cout << "????????????" << error << ":" << error.message()
                    << std::endl;
          std::shared_ptr<SocketInfo> ss =
              NetManager::GetInstance()
                  ->_socket_id_pool[static_cast<uint32_t>(new_id)];
          ss.reset();
        }
      });
}

}  // namespace stunning
