/*
 * tcpclient.c - A simple TCP client
 * usage: tcpclient <host> <port>
 */
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
// 必须的头文件
#include <iostream>
#include <string>
#include <thread>

// #define TCP_HEAD_MAGIC 0b1000101011001110
#define TCP_HEAD_MAGIC 0x54CC
#pragma pack(1)
/// @brief 对tcp传输数据增加消息头
struct TcpMsgHead {
  TcpMsgHead() : _magic(TCP_HEAD_MAGIC), _data_len(0), _action(0) {}
  u_int16_t _magic;
  u_int16_t _data_len;
  u_int16_t _action;
};

#pragma pack()

std::thread::id main_thread_id = std::this_thread::get_id();

void hello(int sockfd) {
  struct epoll_event ev, events[10];
  int nfds, epollfd;
  char *buf[1024];
  epollfd = epoll_create1(0);
  if (epollfd == -1) {
    perror("epoll_create1");
    exit(EXIT_FAILURE);
  }

  ev.events = EPOLLIN | EPOLLOUT;
  ev.data.fd = sockfd;
  if (epoll_ctl(epollfd, EPOLL_CTL_ADD, sockfd, &ev) == -1) {
    perror("epoll_ctl: sockfd");
    exit(EXIT_FAILURE);
  }

  for (;;) {
    nfds = epoll_wait(epollfd, events, 10, -1);
    if (nfds == -1) {
      continue;
    }

    for (int i = 0; i < nfds; ++i) {
      if (EPOLLERR & events[i].events) {
        epoll_ctl(epollfd, EPOLL_CTL_DEL, events[i].data.fd, nullptr);
        close(events[i].data.fd);
      } else {
        std::string output(1024, 0);
        int n = recv(events[i].data.fd, &output[0], 1024, 0);
        std::cout << "n:" << n << ":ret:" << output << std::endl;
      }
    }
  }
}

int main(int argc, char **argv) {
  struct sockaddr_in serveraddr;

  const char *hostname = "127.0.0.1";
  int portno = atoi("9001");

  int sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sockfd < 0) {
    fprintf(stderr, "ERROR opening socket as %s\n", hostname);
    exit(0);
  };

  struct hostent *server = gethostbyname(hostname);
  if (server == NULL) {
    fprintf(stderr, "ERROR, no such host as %s\n", hostname);
    exit(0);
  }

  /* build the server's Internet address */
  bzero((char *)&serveraddr, sizeof(serveraddr));
  serveraddr.sin_family = AF_INET;
  bcopy((char *)server->h_addr, (char *)&serveraddr.sin_addr.s_addr,
        server->h_length);
  serveraddr.sin_port = htons(portno);

  /* connect: create a connection with the server */
  if (connect(sockfd, (struct sockaddr *)&serveraddr, sizeof(serveraddr)) < 0) {
    fprintf(stderr, "ERROR connecting as %s\n", hostname);
    exit(0);
  }

  std::thread a(hello, sockfd);
  a.detach();

  /* get message line from the user */
  printf("Please enter msg: ");
  while (true) {
    // bzero(buf, BUFSIZE);
    // fgets(buf, BUFSIZE, stdin);
    std::string aaa = "";
    std::cin >> aaa;
    /* send the message line to the server */
    TcpMsgHead p;
    p._data_len = aaa.size() + sizeof(TcpMsgHead);
    p._action = 1;

    std::cout << p._magic << ":" << p._data_len << ":" << p._action
              << std::endl;
    int n = send(sockfd, &p, sizeof(p), 0);

    int all_size = aaa.size();
    int par = 10;  // -- 一次发10
    int wait_sleep = 100;
    int need = (all_size % par == 0) ? all_size / par : (all_size / par) + 1;

    for (int a = 0; a < need; a++) {
      int bbb = aaa.size() - a * par;
      int send_size = std::min(par, bbb);
      n = send(sockfd, aaa.c_str() + (a * par), send_size, 0);
      usleep(wait_sleep);  // 睡眠1毫秒?
    }
  }
  close(sockfd);
  return 0;
}