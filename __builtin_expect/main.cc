#include <chrono>
#include <iostream>
#include <string>

#define likely(x) __builtin_expect(!!(x), 1)
#define unlikely(x) __builtin_expect(!!(x), 0)

bool func_normal(int* ary, size_t ary_len, int ans) {
  for (int i = 0; i < ary_len; ++i) {
    if (ary[i] == ans) {
      return true;
    }
  }
  return false;
}

bool func_likely(int* ary, size_t ary_len, int ans) {
  for (int i = 0; i < ary_len; ++i) {
    if (likely(ary[i] == ans)) {
      return true;
    }
  }
  return false;
}

bool func_unlikely(int* ary, size_t ary_len, int ans) {
  for (int i = 0; i < ary_len; ++i) {
    if (unlikely(ary[i] == ans)) {
      return true;
    }
  }
  return false;
}

int main () {
  size_t s = 10000000;
  int *ary = new int[s];
  std::fill(ary, ary+s, 0);

  bool ret = false;
  std::chrono::system_clock::time_point begin;
  std::chrono::system_clock::time_point end;

  begin = std::chrono::system_clock::now();
  ret = func_normal(ary, s, 1);
  end = std::chrono::system_clock::now();

  const auto dur_normal =
    std::chrono::duration_cast<std::chrono::nanoseconds>(end-begin).count();
  std::cout << "Elapsed(msec) = "
       << dur_normal << std::endl;
  std::cout << std::boolalpha << ret << std::endl;

  begin = std::chrono::system_clock::now();
  ret = func_likely(ary, s, 1);
  end = std::chrono::system_clock::now();

  const auto dur_likely =
    std::chrono::duration_cast<std::chrono::nanoseconds>(end-begin).count();
  std::cout << "Elapsed(msec) = "
       << dur_likely << std::endl;
  std::cout << std::boolalpha << ret << std::endl;

  begin = std::chrono::system_clock::now();
  ret = func_unlikely(ary, s, 1);
  end = std::chrono::system_clock::now();
  const auto dur_unlikely =
    std::chrono::duration_cast<std::chrono::nanoseconds>(end-begin).count();
  std::cout << "Elapsed(msec) = "
       << dur_unlikely << std::endl;
  std::cout << std::boolalpha << ret << std::endl;

  std::string fast;
  std::string middle;
  std::string slow;
  if (dur_normal > dur_likely) {
    if (dur_likely > dur_unlikely) {
      fast = "unlikely";
      middle = "likely";
      slow = "normal";
    } else {
       fast = "likely";
       middle = "unlikely";
       slow = "normal";
    }
  } else {
    if (dur_normal > dur_unlikely) {
      fast = "unlikely";
      middle = "normal";
      slow = "likely";
    } else {
      fast = "normal";
      middle = "unlikely";
      slow = "likely";
    }
  }

  std::cout << "fast: " << fast
    << "  middle: " << middle
    << "  slow: " << slow
    << std::endl;

  return 0;
}
