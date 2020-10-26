#include <stdio.h>

void print_array(char *array[], size_t n) {
  printf("[");
  int c;
  for (c = 0; c < n; ++c) {
    printf("%s", array[c]);
    if (c < n - 1) {
      printf(", ");
    }
  }
  printf("]\n");
}

int main() {
  char *strings[] = {"one", "two", "three", "four", "five"};
  size_t n_strings = sizeof(strings) / sizeof(char*);

  int n = 0;                        // 00000
  int n_max = (1 << n_strings) - 1; // 11111
  for (; n <= n_max; ++n) {
    char *combination[n_strings];

    int n_combination = 0;
    int c_strings = 0;
    int bit;  // from 00001 to 10000
    for (bit = 1; c_strings < n_strings; bit <<= 1, ++c_strings) {
      if ((bit & n) == bit) {
        combination[n_combination++] = strings[c_strings];
      }
    }
    print_array(combination, n_combination);
  }
}
