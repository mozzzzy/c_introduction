#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/epoll.h>
#include <errno.h>

#define NEVENTS 16
#define BUFSIZE 1024
#define LISTEN_PORT 10080

enum mystate {
  MYSTATE_READ = 0,
  MYSTATE_WRITE
};

struct clientinfo {
  int fd;
  char buf[BUFSIZE];
  int n;
  int state;
};

int main() {
  // socket の作成
  int sock0 = socket(AF_INET, SOCK_STREAM, 0);
  struct sockaddr_in addr;
  addr.sin_family = AF_INET;
  addr.sin_port = htons(LISTEN_PORT);
  addr.sin_addr.s_addr = INADDR_ANY;
  bind(sock0, (struct sockaddr *)&addr, sizeof(addr));

  // listen 
  listen(sock0, 5);

  // epoll インスタンスを生成
  // int epoll_create(int size)
  // 引数
  //   size は Linux 2.6.8 以降では無視される.
  //   かつては epoll インスタンスが追加しようとする file descriptor の数を kernel に教えるために使われていた.
  // 戻り値
  //   新しい epoll インスタンスを参照する file descriptor を返す.
  int epfd = epoll_create(NEVENTS);
  if (epfd < 0) {
    perror("epoll_create");
    return 1;
  }

  // epoll_event とそのメンバーの epoll_data は以下のような定義
  // struct epoll_event {
  //   uint32_t events;     // epoll event
  //   epoll_data_t data;   // ユーザデータ変数
  // }

  // typedef union epoll_data {
  //   void *ptr;
  //   int  fd;
  //   uint32_t u32;
  //   uint64_t u64;
  // } epoll_data_t;

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

  // epoll_event.data.ptr にメモリ確保して、clientinfo として使用
  ev.data.ptr = calloc(1, sizeof(struct clientinfo));
  if (ev.data.ptr == NULL) {
    perror("calloc");
    return 1;
  }
  ((struct clientinfo *)ev.data.ptr)->fd = sock0;

  // file descriptor epfd が参照する epoll インスタンスに対する処理を行う.
  // 対象 file descriptor fd に対して、操作 op の実行が要求される.
  // int epoll_ctl(int epfd, int op, int fd, struct epoll_event *event)
  // 引数
  //   int op : 指定できる値は以下.
  //     - EPOLL_CTL_ADD : fd を epfd が参照する epoll インスタンスに登録し、
  //                       event を fd に結びついた内部ファイルに関連付ける.
  //                       (event.events に設定した上述の EPOLLIN などのイベントを epoll_wait で待てるようになる.)
  //     - EPOLL_CTL_MOD : event を fd に関連付けるように変更する.
  //     - EPOLL_CTL_DEL : fd を epfd が参照する epoll インスタンスから削除する.
  // 戻り値
  //  成功時: 0
  //  失敗時 -1 (errno を設定する)
  if (epoll_ctl(epfd, EPOLL_CTL_ADD, sock0, &ev) != 0) {
    perror("epoll_ctl");
    return 1;
  }

  for (;;) {
    struct epoll_event ev_ret[NEVENTS];

    // epfd が参照する epoll インスタンスに対するイベントを待つ
    // int epoll_wait(
    //   int epfd,
    //   struct epoll_event *events,
    //   int maxevents,               // epoll_wait で返される epoll_event の最大数
    //   int timeout                  // 単位は milli sec。 -1 の場合 timeout しない。0の場合即時に timeout する
    // )
    // 戻り値
    //   成功時 : 要求されたI/Oに対して準備ができた file descriptor の数を返す
    //   timeout の間に file descriptor が準備できなかった場合 : 0
    //   失敗時 : -1 (errno を設定する)
    int nfds = epoll_wait(epfd, ev_ret, NEVENTS, -1);
    if (nfds < 0) {
      perror("epoll_wait");
      return 1;
    }
    printf("after epoll_wait : nfds=%d\n", nfds);

    // 取得した events 毎の loop
    int i;
    for (i=0; i<nfds; i++) {
      struct clientinfo *ci = ev_ret[i].data.ptr;
      printf("fd=%d\n", ci->fd);

      // 取得した event の data.ptr->fd が sock0 だったら (初回はここに当てはまる)
      if (ci->fd == sock0) {
        // accept
        struct sockaddr_in client;
        socklen_t len = sizeof(client);
        int sock = accept(sock0, (struct sockaddr *)&client, &len);
        if (sock < 0) {
          perror("accept");
          return 1;
        }
        printf("accept sock=%d\n", sock);

        // ev の再利用
        // XXX ev.data.ptr は free しなくて大丈夫か？
        memset(&ev, 0, sizeof(ev));
        ev.events = EPOLLIN | EPOLLONESHOT;
        ev.data.ptr = calloc(1, sizeof(struct clientinfo));
        if (ev.data.ptr == NULL) {
          perror("malloc");
          return 1;
        }
        // ev.data.ptr->fd を accept 済み socket に入れ替え
        ((struct clientinfo *)ev.data.ptr)->fd = sock;

        // epfd の参照する epoll インスタンスに sock に対する ev の設定を追加
        if (epoll_ctl(epfd, EPOLL_CTL_ADD, sock, &ev) != 0) {
          perror("epoll_ctl");
          return 1;
        }
      // accept 済み socket に対する event 検知
      } else {
        // accept 済み socket が read 可能だったら
        if (ev_ret[i].events & EPOLLIN) {
          ci->n = read(ci->fd, ci->buf, BUFSIZE);
          if (ci->n < 0) {
            perror("read");
            return 1;
          }

          ci->state = MYSTATE_WRITE;
          // EPOLLOUT を epoll_wait で取得できるように epoll_ctl
          ev_ret[i].events = EPOLLOUT;
          if (epoll_ctl(epfd, EPOLL_CTL_MOD, ci->fd, &ev_ret[i]) != 0) {
            perror("epoll_ctl");
            return 1;
          }
        // accept 済み socket が write 可能だったら
        } else if (ev_ret[i].events & EPOLLOUT) {
          // read したデータをそのまま write
          int n = write(ci->fd, ci->buf, ci->n);
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

          close(ci->fd);
          free(ev_ret[i].data.ptr);
        }
      }
    }
  }

  close(sock0);
  return 0;
}