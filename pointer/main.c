#include <stdio.h>
#include <stdlib.h>

int main() {
  // pointer
  int *i_ptr = malloc(sizeof(int));
  *i_ptr = 10;
  printf("i_ptr = %d\n", *i_ptr);

  // pointer への pointer
  int **i_ptr_ptr = malloc(sizeof(int*));
  *i_ptr_ptr = malloc(sizeof(int));
  **i_ptr_ptr = 11;
  printf("i_ptr_ptr = %d\n", **i_ptr_ptr);

  return 0;
}
