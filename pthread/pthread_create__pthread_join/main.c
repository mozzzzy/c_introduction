/*
 * pthread_create() と pthread_join() のサンプルコード
 */

#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <unistd.h>

#define SEC_TO_MICRO_SEC 1000 * 1000

// thread で実行する関数
// 引数で指定された秒数 sleep するだけ
void *thread_func(void *sec) {
  printf("thread_func start.\n");
  usleep(*(double *)sec * SEC_TO_MICRO_SEC);
  printf("thread_func finish.\n");
  return sec;
}

int main() {
  // (1) pthread の生成 (pthread_create())
  pthread_t th;
  double sleep_sec = 1.5;
  // int pthread_create(
  //   pthread_t * thread,
  //   pthread_attr_t * attr,           // 生成する thread に適用する thread 属性. NULL だとデフォルトの属性が用いられる.
  //   void * (*start_routine)(void *),
  //   void * arg);
  int err = pthread_create(&th, NULL, thread_func, &sleep_sec);
  if (err == EAGAIN) {
    printf("Got EAGAIN: "
      "No enough resources for new thread, "
      "or More than PTHREAD_THREADS_MAX threads are already active.\n");
    return 1;
  } else if (err != 0) {
    printf("Got unknown error from pthread_create: %d\n", err);
    return 1;
  }

  // (2) thread の終了を待機 (pthread_join())
  void **thread_return;
  // int pthread_join(
  //   pthread_t th,
  //   void **thread_return);   // thread からの戻り値を格納する pointer. 戻り値を使用しない場合 NULL の指定も可能.
  err = pthread_join(th, thread_return);
  if (err != 0) {
    switch (err) {
      case ESRCH:
        printf("Got ESRCH: Couldn't find thread whose id is %ld\n", th);
        break;
      case EINVAL:
        printf("Got EINVAL: "
          "Thread whose id is %ld has already detached, "
          "or other thread has already been waiting for the thread to finish.\n", th);
        break;
      case EDEADLK:
        printf("Got EDEADLK: %ld is the thread id of this thread.\n", th);
        break;
    }
    return 1;
  }

  printf("Return value from thread is %f\n", *(double*)(*thread_return));
  return 0;
}