/*
 * 以下の関数のサンプルコード
 * - pthread_key_create()
 * - pthread_setspecific()
 * - pthread_getspecific()
 * 
 * thread 毎に値の異なるグローバル変数や静的変数が必要な場合に使用する.
 */
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

pthread_key_t key;

// thread が終了した際に TSD 内のデータを destruct する関数.
void free_buffer(void *buf) {
  printf("free_buffer() start.\n");
  free(buf);
  printf("free_buffer() finish.\n");
}

void *thread_func(void *val) {
  printf("thread_func() start.\n");

  // (2) TSD にデータを格納
  int *buf = (int *)calloc(1, sizeof(int));
  *buf = *(int *)val;
  printf("Set value %d to TSD\n", *buf);
  pthread_setspecific(key, buf);

  int *retval = (int *)pthread_getspecific(key);
  printf("Got value from TSD is %d\n", *retval);

  printf("thread_func() finish.\n");
  // (3) thread の終了
  //     ここで (1) で指定した free_buffer() が呼び出される
  pthread_exit(NULL);
}

int main() {
  // (1) thread-specific data (TSD) key の生成
  //   int pthread_key_create(
  //     pthread_key_t *key,                // TSD key
  //     void (*destr_function) (void *));  // destructor func.
  //                                        // thread が pthread_exit や キャンセルによって終了した際に呼び出される.
  pthread_key_create(&key, free_buffer);

  // thread の生成
  pthread_t th;
  int *val = (int *)malloc(sizeof(int));
  *val = 10;
  pthread_create(&th, NULL, thread_func, val);
  pthread_join(th, NULL);

  free(val);
  return 0;
}