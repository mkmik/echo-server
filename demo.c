#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define DEFAULT_PORT 1234
#define MAX_CONN 16
#define MAX_EVENTS 32
#define BUF_SIZE 16
#define MAX_LINE 256

void server_run();
void client_run();

/*
 * register events of fd to epfd
 */
static void epoll_ctl_add(int epfd, int fd, uint32_t events) {
  struct epoll_event ev;
  ev.events = events;
  ev.data.fd = fd;
  if (epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &ev) == -1) {
    perror("epoll_ctl()\n");
    exit(1);
  }
}

static void set_sockaddr(struct sockaddr_in *addr) {
  bzero((char *)addr, sizeof(struct sockaddr_in));
  addr->sin_family = AF_INET;
  addr->sin_addr.s_addr = INADDR_ANY;
  addr->sin_port = htons(DEFAULT_PORT);
}

static int setnonblocking(int sockfd) {
  if (fcntl(sockfd, F_SETFD, fcntl(sockfd, F_GETFD, 0) | O_NONBLOCK) == -1) {
    return -1;
  }
  return 0;
}

/*
 * epoll echo server
 */
void main() {
  int i;
  int n;
  int epfd;
  int nfds;
  int listen_sock;
  int conn_sock;
  int socklen;
  char buf[BUF_SIZE];
  struct sockaddr_in srv_addr;
  struct sockaddr_in cli_addr;
  struct epoll_event events[MAX_EVENTS];

  listen_sock = socket(AF_INET, SOCK_STREAM, 0);
  if (listen_sock < 0) {
    perror("socket() failed");
    exit(1);
  }

  if (setsockopt(listen_sock, SOL_SOCKET, SO_REUSEADDR, &(int){1},
                 sizeof(int)) < 0) {
    perror("setsockopt(SO_REUSEADDR) failed");
    exit(1);
  }

  set_sockaddr(&srv_addr);
  if (bind(listen_sock, (struct sockaddr *)&srv_addr, sizeof(srv_addr)) < 0) {
    perror("cannot bind");
    exit(1);
  }

  setnonblocking(listen_sock);
  if (listen(listen_sock, MAX_CONN) < 0) {
    perror("cannot listen");
  }
  printf("listening on port %d\n", DEFAULT_PORT);

  epfd = epoll_create(1);
  epoll_ctl_add(epfd, listen_sock, EPOLLIN | EPOLLOUT | EPOLLET);

  socklen = sizeof(cli_addr);
  for (;;) {
    nfds = epoll_wait(epfd, events, MAX_EVENTS, -1);
    for (i = 0; i < nfds; i++) {
      if (events[i].data.fd == listen_sock) {
        /* handle new connection */
        conn_sock = accept(listen_sock, (struct sockaddr *)&cli_addr, &socklen);

        inet_ntop(AF_INET, (char *)&(cli_addr.sin_addr), buf, sizeof(cli_addr));
        printf("[+] connected with %s:%d\n", buf, ntohs(cli_addr.sin_port));

        setnonblocking(conn_sock);
        epoll_ctl_add(epfd, conn_sock,
                      EPOLLIN | EPOLLET | EPOLLRDHUP | EPOLLHUP);
      } else if (events[i].events & EPOLLIN) {
        /* handle EPOLLIN event */
        for (;;) {
          bzero(buf, sizeof(buf));
          n = read(events[i].data.fd, buf, sizeof(buf));
          if (n <= 0 /* || errno == EAGAIN */) {
            break;
          } else {
            printf("[+] data: %s\n", buf);
            write(events[i].data.fd, buf, strlen(buf));
          }
        }
      } else {
        printf("[+] unexpected\n");
      }
      /* check if the connection is closing */
      if (events[i].events & (EPOLLRDHUP | EPOLLHUP)) {
        printf("[+] connection closed\n");
        epoll_ctl(epfd, EPOLL_CTL_DEL, events[i].data.fd, NULL);
        close(events[i].data.fd);
        continue;
      }
    }
  }
}
