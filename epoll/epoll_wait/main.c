#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/epoll.h>
#include <errno.h>

#define N_EVENTS 16
#define BUFSIZE 1024
#define EPOLL_TIMEOUT_MSEC 1000
#define LISTEN_PORT 10080


enum mystate {
  MYSTATE_BEFORE_ACCEPT = 0,
  MYSTATE_BEFORE_READ,
  MYSTATE_BEFORE_WRITE
};

struct clientinfo {
  int  fd;
  char buf[BUFSIZE];  // client から受信したデータを格納する buffer
  int  n_read_write;
  int  state;
};

// port 番号を渡して、socket を生成、bind、listen を行う関数
int listen_at(int port) {
  // socket の作成
  int sock = socket(AF_INET, SOCK_STREAM, 0);

  // LISTEN_PORT に bind
  struct sockaddr_in addr;
  addr.sin_family = AF_INET;
  addr.sin_port = htons(port);
  addr.sin_addr.s_addr = INADDR_ANY;
  bind(sock, (struct sockaddr *)&addr, sizeof(addr));

  // listen 
  listen(sock, 5);

  return sock;
}

int main() {
  int sock0 = listen_at(LISTEN_PORT);

  // int epoll_create(int size)
  //  epoll インスタンスを生成
  //   [引数]
  //   - int size : size は Linux 2.6.8 以降では無視される.
  //              かつては epoll インスタンスが追加しようとする file descriptor の数を kernel に教えるために使われていた.
  //   [戻り値]
  //   - int : 新しい epoll インスタンスを参照する file descriptor を返す.
  int epfd = epoll_create(N_EVENTS);
  if (epfd < 0) {
    perror("epoll_create");
    return 1;
  }

  // struct epoll_event ev を生成する。
  // epoll_event は以下のような定義
  //
  // struct epoll_event {
  //   uint32_t events;     // epoll event (EPOLLIN(read操作が可能), EPOLLOUT(writeそうsが可能) など)
  //   epoll_data_t data;   // ユーザデータ変数
  // }
  //
  // また epoll_data_t は以下のような定義
  //
  // typedef union epoll_data {
  //   void *ptr;
  //   int  fd;
  //   uint32_t u32;
  //   uint64_t u64;
  // } epoll_data_t;
  //
  // ev は後の処理で epoll_ctl(epfd, EPOLL_CTL_ADD, sock0, &ev) のように渡される。
  // これによって、epoll_wait(epfd, ev_ret, N_EVENTS, -1); した際に、sock0 への read操作が可能だった場合、
  // struct epoll_event ev_ret[i] に値が入り、その ev_ret[i].data はこの ev の ev.data と同じデータになる。
  struct epoll_event ev;
  memset(&ev, 0, sizeof(ev));

  // epoll_event.events は、
  // 以下のような使用可能なイベントタイプを使って構成されたビットセット.
  //   EPOLLIN      : 関連付けられた file descriptor に read 操作が可能である
  //   EPOLLOUT     : 関連付けられた file descriptor に write 操作が可能である
  //   EPOLLONESHOT : 関連付けられた file descriptor に One-Shot behavior を設定する.
  //                  これはイベントが epoll_wait で呼び出された後、関連付けられた file descriptor が破棄され、
  //                  インタフェースによってイベントが報告されなくなることを意味する.
  //   (他にもたくさん)
  ev.events = EPOLLIN;

  // epoll_event.data.ptr に clientinfo 構造体のサイズ分メモリ確保して、0 で埋める.
  ev.data.ptr = calloc(1, sizeof(struct clientinfo));
  if (ev.data.ptr == NULL) {
    perror("calloc");
    return 1;
  }
  printf("address of created cleintinfo: %x\n", ev.data.ptr);
  ((struct clientinfo *)ev.data.ptr)->fd = sock0;
  ((struct clientinfo *)ev.data.ptr)->state = MYSTATE_BEFORE_ACCEPT;

  // この時点で
  // ev は以下のようになっている。
  // この ev.data は epoll_wait() によって epoll_event を取得する際に、まったく同じものが受け渡される。
  // ev = struct epoll_event {
  //   uint32_t events;     == EPOLLIN
  //   epoll_data_t data;   == union epoll_data {
  //                             void *ptr   == struct clientinfo {
  //                                              int fd   == sock0
  //                                            }
  //                           }
  // }

  // int epoll_ctl(
  //   int epfd,
  //   int op,
  //   int fd,
  //   struct epoll_event *event
  // )
  //  file descriptor epfd が参照する epoll インスタンスに対する処理を行う.
  //  対象 file descriptor fd に対して、操作 op の実行が要求される.
  //   [引数]
  //   - int op : 指定できる値は以下.
  //       - EPOLL_CTL_ADD : fd を epfd が参照する epoll インスタンスに登録し、
  //                         event を fd に結びついた内部ファイルに関連付ける.
  //                         (event.events に設定した上述の EPOLLIN などのイベントを epoll_wait で待てるようになる.)
  //       - EPOLL_CTL_MOD : event を fd に関連付けるように変更する.
  //       - EPOLL_CTL_DEL : fd を epfd が参照する epoll インスタンスから削除する.
  //   [戻り値]
  //    - 成功時: 0
  //    - 失敗時: -1 (errno を設定する)
  if (epoll_ctl(epfd, EPOLL_CTL_ADD, sock0, &ev) != 0) {
    perror("epoll_ctl");
    return 1;
  }

  struct epoll_event ev_ret[N_EVENTS];
  for (;;) {
    // int epoll_wait(
    //   int epfd,
    //   struct epoll_event *events,
    //   int maxevents,               // epoll_wait で返される epoll_event の最大数
    //   int timeout                  // 単位は milli sec。 -1 の場合 timeout しない。0の場合即時に timeout する
    // )
    //  epfd が参照する epoll インスタンスに対するイベントを待つ。
    //  events が指すメモリー領域には、 呼び出し側が利用可能なイベントが格納される。
    //
    //   [戻り値]
    //     成功時                                              : 要求されたI/Oに対して準備ができた file descriptor の数を返す
    //     timeout の間に file descriptor が準備できなかった場合 : 0
    //     失敗時                                              : -1 (errno を設定する)
    int n_fds = epoll_wait(epfd, ev_ret, N_EVENTS, EPOLL_TIMEOUT_MSEC);
    if (n_fds < 0) {
      perror("epoll_wait");
      return 1;
    }
    if (n_fds == 0) {
      // epoll_wait timeout.
      printf("epoll_wait waited %d milli second and timeout. Wait again.\n", EPOLL_TIMEOUT_MSEC);
      continue;
    }

    printf("Got epoll_events from epoll_wait. %d file descriptors are ready for operation.\n", n_fds);

    // 取得した events 毎の loop
    int i;
    for (i=0; i<n_fds; i++) {
      struct clientinfo* ci = (struct clientinfo*)(ev_ret[i].data.ptr);
      int my_state = ci->state;
      printf("ready sock: %d\n", ci->fd);

      switch (my_state) {
        case MYSTATE_BEFORE_ACCEPT:
          printf("state : BEFORE_ACCEPT.\n");
          // ev_ret[i].data は ev.data とアドレスも中身も等しくなる
          printf("address of got epoll_event.data: %x\n", ev_ret[i].data);

          // accept
          struct sockaddr_in client;
          socklen_t len = sizeof(client);
          int sock = accept(sock0, (struct sockaddr *)&client, &len);
          if (sock < 0) {
            perror("accept");
            return 1;
          }
         
          // accept 済み socket の epoll_wait による監視を追加 (ev変数は再利用)
          ev.events = EPOLLIN;
          ev.data.ptr = calloc(1, sizeof(struct clientinfo));
          if (ev.data.ptr == NULL) {
            perror("calloc");
            return 1;
          }
          ((struct clientinfo*)(ev.data.ptr))->fd = sock;
          ((struct clientinfo*)(ev.data.ptr))->state = MYSTATE_BEFORE_READ;

          if (epoll_ctl(epfd, EPOLL_CTL_ADD, sock, &ev) != 0) {
            perror("epoll_ctl");
            return 1;
          }
          break;
        case MYSTATE_BEFORE_READ:
          printf("state : MYSTATE_BEFORE_READ.\n");

          // read
          ci->n_read_write = read(ci->fd, ci->buf, BUFSIZE);
          if (ci->n_read_write < 0) {
            perror("read");
            return 1;
          }
          ci->state = MYSTATE_BEFORE_WRITE;

          // EPOLLOUT を epoll_wait で取得できるように epoll_ctl
          ev_ret[i].events = EPOLLOUT;
          if (epoll_ctl(epfd, EPOLL_CTL_MOD, ci->fd, &ev_ret[i]) != 0) {
            perror("epoll_ctl");
            return 1;
          }
          break;
        case MYSTATE_BEFORE_WRITE:
          printf("state : MYSTATE_BEFORE_WRITE.\n");

          // write
          int n = write(ci->fd, ci->buf, ci->n_read_write);
          if (n < 0) {
            perror("write");
            return 1;
          }

          // fd を epfd が参照する epoll インスタンスから削除する.
          // event 引数は無視されるので NULL でよい.
          if (epoll_ctl(epfd, EPOLL_CTL_DEL, ci->fd, NULL) != 0) {
            perror("epoll_ctl");
            return 1;
          }

          // accept 済みの socket を close
          close(ci->fd);
          // clientinfo を free
          free(ev_ret[i].data.ptr);
          break;
      } // end of switch
    } // end of epd loop
  } // end of accept loop 

  close(sock0);
  return 0;
}
